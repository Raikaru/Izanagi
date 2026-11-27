#!/usr/bin/env python3
"""Cross-platform doctest XML identity and failure-preservation regressions."""
import re
import tempfile
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

from upstream_suite import DEFAULT_COMPILER_CASES, alignment_result, api_host_optimization, combined_status, fixture_result, gc_semantic_result, inliner_fixture, native_only, partial_case_result, scoped_xml_results, subcase_observed, xml_results
from fastcall_differential import compare_output, dense_case, ulp_distance
from upstream_inventory import inventory


def write_report(directory, rows, *, summary=None):
    root = ET.Element('doctest')
    suites = {}
    for filename, line, suite_name, case_name, success in rows:
        if suite_name not in suites:
            suites[suite_name] = ET.SubElement(root, 'TestSuite', name=suite_name)
        case = ET.SubElement(suites[suite_name], 'TestCase', filename=filename, line=str(line), name=case_name)
        attrs = {'successes': '1' if success else '0', 'failures': '0' if success else '1'}
        if success is not None:
            attrs['test_case_success'] = 'true' if success else 'false'
        ET.SubElement(case, 'OverallResultsAsserts', attrs)
    if summary is None:
        passed = sum(success is True for *_, success in rows)
        failed = sum(success is False for *_, success in rows)
        summary = (passed, failed)
    ET.SubElement(root, 'OverallResultsTestCases', successes=str(summary[0]), failures=str(summary[1]))
    path = Path(directory) / 'report.xml'
    ET.ElementTree(root).write(path, encoding='utf-8', xml_declaration=True)
    return path


class DoctestXmlTests(unittest.TestCase):
    def test_unix_windows_and_generated_sources_match_inventory_names(self):
        rows = [
            ('/work/src/tests/Conformance.test.cpp', 1463, 'Conformance', 'JitInliner', True),
            (r'C:\agent\src\tests\Conformance.test.cpp', 3899, 'Conformance', 'Interrupt', False),
            ('Conformance.test.cpp', 5635, 'Conformance', 'ClassInheritanceRepeatedCallMemberOffsetCorruption', True),
            (r'D:\build\tests\DirectFieldAccess.test.cpp', 61, 'DirectFieldAccess', 'handler', True),
        ]
        with tempfile.TemporaryDirectory() as directory:
            results = xml_results(write_report(directory, rows))
        self.assertEqual(len(results), 4)
        for filename, line, suite_name, case_name, success in rows:
            basename = filename.replace('\\', '/').rsplit('/', 1)[-1]
            result = results[(basename, line, suite_name, case_name)]
            self.assertEqual(result['filename'], filename)
            self.assertEqual(result['pass'], success)
        self.assertFalse(results[('Conformance.test.cpp', 3899, 'Conformance', 'Interrupt')]['pass'])
        self.assertEqual(results[('Conformance.test.cpp', 3899, 'Conformance', 'Interrupt')]['assert_failures'], 1)

    def test_ambiguous_normalized_identity_is_an_error(self):
        rows = [('/first/Conformance.test.cpp', 1, 'Conformance', 'Same', True),
                (r'C:\second\Conformance.test.cpp', 1, 'Conformance', 'Same', True)]
        with tempfile.TemporaryDirectory() as directory:
            report = write_report(directory, rows)
            with self.assertRaisesRegex(ValueError, 'duplicate doctest identity after filename normalization'):
                xml_results(report)

    def test_missing_status_and_inconsistent_summary_fail_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            report = write_report(directory, [('Conformance.test.cpp', 1, 'Conformance', 'Unknown', None)])
            with self.assertRaisesRegex(ValueError, 'missing or invalid test-case outcome'):
                xml_results(report)
        with tempfile.TemporaryDirectory() as directory:
            report = write_report(directory, [('Conformance.test.cpp', 1, 'Conformance', 'Failed', False)], summary=(1, 0))
            with self.assertRaisesRegex(ValueError, 'summary does not match reported cases'):
                xml_results(report)

    def test_real_filtered_doctest_report_keeps_only_executed_case(self):
        report = Path(__file__).with_name('upstream-api-skipped-actual.xml')
        expected = ('Conformance.test.cpp', 3284, 'Conformance', 'ApiAlloc')
        results = scoped_xml_results(report, [expected])
        self.assertEqual(set(results), {expected})
        self.assertTrue(results[expected]['pass'])
        self.assertEqual(results[expected]['assert_successes'], 2)
        self.assertEqual(results[expected]['assert_failures'], 0)
        # A filtered-out case is not an executed PASS even when other tests pass.
        skipped = ('Conformance.test.cpp', 3255, 'Conformance', 'ApiStack')
        with self.assertRaisesRegex(ValueError, 'case identities differ'):
            scoped_xml_results(report, [skipped])

    def test_executed_missing_results_and_contradictory_skips_fail_closed(self):
        fixture = Path(__file__).with_name('upstream-api-skipped-actual.xml')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'altered.xml'
            tree = ET.parse(fixture)
            case = tree.find(".//TestCase[@name='ApiAlloc']")
            case.remove(case.find('OverallResultsAsserts'))
            tree.write(path, encoding='utf-8', xml_declaration=True)
            with self.assertRaisesRegex(ValueError, 'missing result'):
                xml_results(path)

            tree = ET.parse(fixture)
            case = tree.find(".//TestCase[@name='ApiAlloc']")
            case.set('skipped', 'true')
            tree.write(path, encoding='utf-8', xml_declaration=True)
            with self.assertRaisesRegex(ValueError, 'skipped doctest case has result content'):
                xml_results(path)

            tree = ET.parse(fixture)
            tree.find('OverallResultsTestCases').set('skipped', '38')
            tree.write(path, encoding='utf-8', xml_declaration=True)
            with self.assertRaisesRegex(ValueError, 'skipped summary does not match'):
                xml_results(path)

class ApiOptimizationTests(unittest.TestCase):
    def test_actual_host_configuration_is_not_multiplied_across_variants(self):
        cases = {case['name']: case for case in inventory() if case['suite'] == 'Conformance'}
        for name in ('ApiStack', 'ApiEncode', 'UserdataAlignment', 'SandboxFreezesVectorMetatable'):
            self.assertIsNone(api_host_optimization(cases[name]), name)
            self.assertNotIn('luau_compile(', cases[name]['source'])
            self.assertNotIn('runConformance(', cases[name]['source'])
        for name in ('ApiCalls', 'Interrupt', 'Debugger', 'InterruptInspection',
                     'InterruptErrorInspection', 'HugeFunctionLoadFailure', 'HugeFunction',
                     'HugeConstantTable', 'LargeNestedClosure', 'UserdataDirectAccess',
                     'ClassInheritanceRepeatedCallMemberOffsetCorruption'):
            self.assertEqual(api_host_optimization(cases[name]), 1, name)
        self.assertEqual(api_host_optimization(cases['NDebugGetUpValue']), 0)
        self.assertIn('copts.optimizationLevel = 0;', cases['NDebugGetUpValue']['source'])
        self.assertEqual(api_host_optimization({'suite': 'DirectFieldAccess', 'name': 'handler'}), 1)

    def test_fixed_default_cases_really_use_default_compile_options(self):
        cases = {case['name']: case for case in inventory() if case['suite'] == 'Conformance'}
        for name in DEFAULT_COMPILER_CASES:
            self.assertIn('nullptr, &bytecodeSize)', cases[name]['body'], name)
            self.assertNotIn('runConformance(', cases[name]['body'], name)

class NativeClassificationTests(unittest.TestCase):
    def test_only_native_suites_are_classified_without_running(self):
        cases = inventory()
        lowering = [case for case in cases if case['suite'] == 'IrLowering']
        allocator = [case for case in cases if case['suite'] == 'SharedCodeAllocator']
        feedback = [case for case in cases if case['suite'] == 'FeedbackVector']
        self.assertEqual(len(lowering), 203)
        self.assertEqual(len(allocator), 5)
        self.assertTrue(feedback)
        for case in lowering:
            self.assertIn('getCodegen', case['body'], case['name'])
            self.assertTrue(native_only(case), case['name'])
        for case in allocator:
            self.assertIn('luau_codegen_supported', case['body'], case['name'])
            self.assertTrue(native_only(case), case['name'])
        for case in feedback:
            self.assertFalse(native_only(case), case['name'])

    def test_feedback_cases_have_one_unchanged_silent_script(self):
        feedback = [case for case in inventory() if case['suite'] == 'FeedbackVector']
        self.assertEqual(len(feedback), 8)
        for case in feedback:
            scripts = re.findall(r'compile\(R"\((.*?)\)"\);', case['source'], re.DOTALL)
            self.assertEqual(len(scripts), 1, case['name'])
            self.assertNotRegex(scripts[0], r'\bprint\s*\(')

    def test_bytecode_utility_script_inventory(self):
        cases = {case['name']: case for case in inventory() if case['suite'] == 'Conformance'}
        source = cases['BytecodeDistributionPerFunctionTest']['source']
        scripts = re.findall(r'const char\* source = R"\((.*?)\)";', source, re.DOTALL)
        self.assertEqual(len(scripts), 1)
        self.assertNotRegex(scripts[0], r'\bprint\s*\(')
        self.assertNotIn('runConformance(', cases['BytecodeDumpLinked']['source'])
        self.assertNotIn('luau_compile(', cases['BytecodeDumpLinked']['source'])

    def test_long_jump_fixture_matches_upstream_builder(self):
        case = next(case for case in inventory() if case['name'] == 'JitInlinerLongJump')
        source = inliner_fixture(case['source'])
        self.assertEqual(source.count('    n = n + 1\n'), 40000)
        self.assertTrue(source.endswith('return s\n'))
        with self.assertRaises(ValueError):
            inliner_fixture(case['source'].replace('source += "return s\\n";', 'unknown_statement();'))

class FixtureResultTests(unittest.TestCase):
    def test_reference_failure_is_not_hidden_by_candidate_skip(self):
        ok = {'exit': 0, 'stdout': 'OK\n', 'stderr': ''}
        failed = {'exit': 1, 'stdout': '', 'stderr': 'fixture setup failed'}
        skipped = {'exit': 77, 'stdout': '', 'stderr': 'unsupported'}
        self.assertEqual(fixture_result(ok, ok)[0], 'PASS')
        self.assertEqual(fixture_result(failed, skipped)[0], 'FAIL')
        self.assertIn('reference', fixture_result(failed, skipped)[1])
        self.assertEqual(fixture_result(ok, failed)[0], 'FAIL')
        self.assertIn('candidate', fixture_result(ok, failed)[1])
        self.assertEqual(fixture_result(ok, skipped)[0], 'SKIP')

    def test_gc_semantic_witness_preserves_failures_and_skips(self):
        passed = {'pass': True}
        failed = {'pass': False}
        self.assertEqual(gc_semantic_result(passed, 0, 0), 'PASS')
        self.assertEqual(gc_semantic_result(None, 0, 0), 'SKIP')
        self.assertEqual(gc_semantic_result(passed, 77, 0), 'SKIP')
        self.assertEqual(gc_semantic_result(passed, 0, None), 'SKIP')
        self.assertEqual(gc_semantic_result(failed, 0, 77), 'FAIL')
        self.assertEqual(gc_semantic_result(passed, 1, 77), 'FAIL')
        self.assertEqual(gc_semantic_result(passed, 77, 1), 'FAIL')

    def test_promoted_alignment_assertions_require_both_complete_results(self):
        reference = {'pass': True}
        passed = {'pass': True, 'assert_successes': 10, 'assert_failures': 0}
        missing_assertions = {'pass': True, 'assert_successes': 0, 'assert_failures': 0}
        shorter = {'pass': True, 'assert_successes': 9, 'assert_failures': 0}
        failed = {'pass': False, 'assert_successes': 9, 'assert_failures': 1}
        self.assertEqual(alignment_result(reference, passed, passed), 'PASS')
        self.assertEqual(alignment_result(reference, passed, None), 'SKIP')
        self.assertEqual(alignment_result(reference, failed, None), 'FAIL')
        self.assertEqual(alignment_result(reference, missing_assertions, missing_assertions), 'FAIL')
        self.assertEqual(alignment_result(reference, passed, shorter), 'FAIL')

    def test_aggregate_subcase_preserves_missing_and_failed_variants(self):
        self.assertEqual(combined_status(['PASS', 'PASS', 'PASS']), 'PASS')
        self.assertEqual(combined_status(['PASS', 'SKIP']), 'SKIP')
        self.assertEqual(combined_status(['SKIP', 'FAIL']), 'FAIL')
        self.assertEqual(combined_status([]), 'SKIP')

    def test_partial_case_requires_graphs_gc_and_matching_assertions(self):
        ref = {'pass': True}
        host = {'pass': True, 'assert_successes': 5, 'assert_failures': 0}
        fewer = {'pass': True, 'assert_successes': 4, 'assert_failures': 0}
        graph = ('PASS', 2)
        self.assertEqual(partial_case_result(ref, host, host, graph, graph, 'PASS'), 'PASS')
        self.assertEqual(partial_case_result(ref, host, host, graph, None, 'PASS'), 'SKIP')
        self.assertEqual(partial_case_result(None, host, host, ('FAIL', 1), None, 'PASS'), 'FAIL')
        self.assertEqual(partial_case_result(ref, host, fewer, graph, graph, 'PASS'), 'FAIL')
        self.assertEqual(partial_case_result(ref, host, host, graph, ('PASS', 1), 'PASS'), 'FAIL')
        self.assertEqual(partial_case_result(ref, host, host, graph, graph, 'FAIL'), 'FAIL')

    def test_subcase_path_requires_reported_nested_execution(self):
        result = {'xml': '<TestCase><SubCase name="parent"><SubCase name="O0"/></SubCase>'
                         '<SubCase name="parent"><SubCase name="O1"/></SubCase></TestCase>'}
        self.assertTrue(subcase_observed(result, ['parent']))
        self.assertTrue(subcase_observed(result, ['parent', 'O1']))
        self.assertFalse(subcase_observed(result, ['parent', 'O2']))
        self.assertFalse(subcase_observed({'xml': '<invalid'}, ['parent']))
        self.assertFalse(subcase_observed(None, ['parent']))

    def test_fastcall_ulp_is_limited_to_numeric_results(self):
        one = 'result=number:3ff0000000000000'
        adjacent = 'result=number:3ff0000000000001'
        two_away = 'result=number:3ff0000000000002'
        self.assertEqual(ulp_distance(0x3ff0000000000000, 0x3ff0000000000001), 1)
        self.assertEqual(compare_output(one, adjacent, True), (True, 1))
        self.assertEqual(compare_output(one, two_away, True), (False, 2))
        self.assertEqual(compare_output(one, adjacent, False), (False, 0))
        self.assertFalse(compare_output('global=x:' + one, 'global=x:' + adjacent, True)[0])
        left = 'result=table:{number:3ff0000000000000=number:3ff0000000000000;}'
        right = 'result=table:{number:3ff0000000000000=number:3ff0000000000001;}'
        self.assertEqual(compare_output(left, right, True), (True, 1))
        wrong_key = 'result=table:{number:4000000000000000=number:3ff0000000000001;}'
        self.assertFalse(compare_output(left, wrong_key, True)[0])
        self.assertFalse(compare_output('result=number:0000000000000000',
                                        'result=number:8000000000000000', True)[0])
        self.assertFalse(compare_output('result=number:7ff0000000000000',
                                        'result=number:7ff0000000000001', True)[0])
        classes = {'exact': 0, 'finite_ulp': 0, 'nan_divergence': 0, 'failure': 0}
        self.assertTrue(compare_output('result=number:7ff8000000000001',
                                       'result=number:fff8000000000002', True, classes)[0])
        self.assertEqual(classes['nan_divergence'], 1)
        self.assertEqual(dense_case(24, 8), dense_case(24, 8))



if __name__ == '__main__':
    unittest.main()
