#!/usr/bin/env python3
"""Regression coverage for the pinned upstream C++ test inventory parser."""
import unittest

from upstream_inventory import closing_brace, inventory


class BraceScannerTests(unittest.TestCase):
    def test_digit_separators_and_noncode_braces(self):
        source = r'''{
            int count = 1'000;
            unsigned mask = 0xDE'AD;
            double number = 1.2'3e+4;
            auto a = '{';
            auto b = "ignore } and \\\" quotes";
            auto c = u8R"tag(raw } and " quotation)tag";
            // } and ' must not affect nesting
            /* { ' } */
            if (count) { count++; }
        } // end
        TEST_CASE("following") {}
        '''
        end = closing_brace(source, 0)
        self.assertEqual(end, source.index('} // end') + 1)
        self.assertIn('TEST_CASE("following")', source[end:])

    def test_unclosed_block_is_not_silently_accepted(self):
        with self.assertRaisesRegex(ValueError, 'unterminated C\\+\\+ block'):
            closing_brace('{ if (true) { return; }', 0)

    def test_pinned_cases_and_nested_subcases_are_retained(self):
        cases = inventory()
        selected = {c['name']: c for c in cases if c['file'] == 'Conformance.test.cpp'
                    and c['name'] in {'JitInliner', 'Interrupt', 'IrInstructionLimit', 'Vector', 'JitInlinerLongJump'}}
        self.assertEqual(set(selected), {'JitInliner', 'Interrupt', 'IrInstructionLimit', 'Vector', 'JitInlinerLongJump'})
        self.assertEqual(selected['JitInliner']['line'], 1463)
        self.assertEqual([c['path'] for c in selected['JitInliner']['subcases']], [['Default'], ['RecordCounters']])
        self.assertEqual([c['file'] for c in selected['JitInliner']['calls']], ['jit_inliner.luau'])
        self.assertEqual(selected['Interrupt']['line'], 3899)
        self.assertEqual([c['file'] for c in selected['Interrupt']['calls']], ['interrupt.luau'])
        self.assertEqual(selected['IrInstructionLimit']['line'], 5298)
        self.assertEqual(selected['JitInlinerLongJump']['line'], 5698)
        self.assertEqual([c['path'] for c in selected['Vector']['subcases']], [
            ['NoIrHooks'], ['NoIrHooks', 'O0'], ['NoIrHooks', 'O1'], ['NoIrHooks', 'O2'],
            ['IrHooks'], ['IrHooks', 'O0'], ['IrHooks', 'O1'], ['IrHooks', 'O2'],
        ])


if __name__ == '__main__':
    unittest.main()
