#!/usr/bin/env python3
"""Exercise actual Golden container/FFmpeg/mixer paths with synthetic bytes."""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import wave
import golden_audio_fixtures as fx

COMMAND = sys.argv[1:]
sys.argv[1:] = []


class AudioTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='golden-audio-')
        self.root = Path(self.temp.name)
        self.env = dict(os.environ, P4PC_CONFIG='none', P4PC_MODE='golden', P4PC_INPUT='script')

    def tearDown(self):
        self.temp.cleanup()

    def put(self, name, data):
        path = self.root / name
        path.write_bytes(data)
        return str(path)

    def run_cli(self, *args, good=True, env=None):
        result = subprocess.run(COMMAND + list(map(str,args)), capture_output=True, env=env or self.env, timeout=30)
        self.assertEqual(result.returncode == 0, good, result.stderr.decode(errors='replace'))
        return json.loads(result.stdout) if good and result.stdout.startswith(b'{') else result

    def test_acb_graph_reuses_utf_and_projects_unused_header_work(self):
        for work in (0, 88):
            result = self.run_cli('--inspect', self.put('input.acb',fx.acb(extra_work=work)))
            self.assertEqual(result['tables'][:7], [1]*7)
            self.assertEqual(result['hca'],1)
            cue = result['cues'][0]
            self.assertEqual(cue['id'],19)
            self.assertEqual(cue['waves'][0]['id'],77)
            self.assertEqual(cue['waves'][0]['samples'],3904)
        layered = self.run_cli('--inspect',self.put('layers.acb',fx.acb(layers=2)))
        self.assertEqual(len(layered['cues'][0]['waves']),2)

    def test_awb_widths_alignment_empty_and_duplicate_ids(self):
        for ow in (2,4,8):
            for iw in (2,4):
                result = self.run_cli('--awb',self.put('input.awb',fx.awb([(9,fx.hca()),(2,fx.hca())],ow,iw)))
                self.assertEqual((result['bank_waves'],result['hca']),(2,2))
        self.assertEqual(self.run_cli('--awb',self.put('empty.awb',fx.awb([])))['bank_waves'],0)
        self.run_cli('--awb',self.put('duplicate.awb',fx.awb([(7,fx.hca()),(7,fx.hca())])),good=False)
        bad = bytearray(fx.awb([(1,fx.hca())]));struct.pack_into('<I',bad,18,0)
        cpk = self.run_cli('--awb',self.put('itoc.awb',fx.itoc_bank([(7,fx.hca()),(2,fx.hca())])))
        self.assertEqual((cpk['bank_waves'],cpk['hca']),(2,2))
        self.run_cli('--awb',self.put('bad.awb',bad),good=False)

    def test_malformed_tables_cycles_and_bounded_graph(self):
        self.run_cli('--inspect',self.put('cycle.acb',fx.acb(cycle=True)),good=False)
        self.run_cli('--inspect',self.put('too-many.acb',fx.acb(layers=65)),good=False)
        original = fx.acb()
        for n in (0,1,8,31,len(original)-1):
            self.run_cli('--inspect',self.put('truncated.acb',original[:n]),good=False)
        bad = bytearray(original);struct.pack_into('>I',bad,4,len(bad)+500)
        self.run_cli('--inspect',self.put('bad.acb',bad),good=False)

    def test_hca_trim_crc_and_synthetic_decoder(self):
        path = self.put('silence.hca',fx.hca())
        out = self.root/'decode.wav'
        result = self.run_cli('--decode',path,out)
        self.assertEqual((result['frames'],result['channels'],result['rate']),(3904,1,44100))
        with wave.open(str(out),'rb') as wav:
            self.assertEqual(wav.getnframes(),3904)
            self.assertEqual(len(wav.readframes(5000)),3904*2)
        bad = bytearray(fx.hca());bad[20]^=1
        self.run_cli('--decode',self.put('bad-header.hca',bad),self.root/'bad.wav',good=False)
        self.assertFalse((self.root/'bad.wav').exists())
        bad = bytearray(fx.hca());bad[-4]^=1
        self.run_cli('--decode',self.put('bad-packet.hca',bad),self.root/'corrupt.wav',good=False)
        self.assertFalse((self.root/'corrupt.wav').exists())

    def test_keyed_refusal_and_private_setting_to_real_ffmpeg_extradata(self):
        path = self.put('keyed.hca',fx.hca(cipher=56))
        out = self.root/'keyed.wav'
        refused = self.run_cli('--decode',path,out,good=False)
        self.assertIn(b'requires the private golden.hca_key setting',refused.stderr)
        self.assertFalse(out.exists())
        config = self.put('private.ini',b'[golden]\nhca_key=0x123456789abcdef0\n')
        env = dict(self.env,P4PC_CONFIG=config)
        decoded = self.run_cli('--decode',path,out,env=env)
        self.assertEqual(decoded['frames'],3904)
        # The synthetic zero/FF spectral packet is invariant under HCA's key
        # permutation; this exercises extradata, not a game key or key recovery.
        config = self.put('private.ini',b'[golden]\nhca_key=invalid-private-fixture\n')
        result = self.run_cli('--decode',path,self.root/'invalid.wav',good=False,env=dict(env,P4PC_CONFIG=config))
        self.assertNotIn(b'invalid-private-fixture',result.stderr+result.stdout)

    def test_independent_stream_group_sums_starts_pauses_and_drains(self):
        for option, delay in (('--group-mix',0),('--group-delay',8)):
            result = self.run_cli(option)
            self.assertEqual(result, {'group_voices': 2, 'frames_per_voice': 32, 'delay_frames': delay})

    def test_existing_mixer_and_mode_isolation(self):
        acb = self.put('memory.acb',fx.acb())
        for category in (0,1,2):
            result = self.run_cli('--mix',acb,'-',19,category,self.root/('mix%d.wav'%category))
            self.assertEqual((result['rate'],result['channels']),(48000,2))
            self.assertLess(abs(result['seconds']-3904/44100),1/48000)
        external = self.put('stream.acb',fx.acb(streaming=True))
        bank = self.put('stream.awb',fx.awb([(77,fx.hca())]))
        self.run_cli('--mix',external,bank,19,0,self.root/'stream.wav')
        result = subprocess.run(COMMAND+['--ps2'],env=dict(self.env,P4PC_MODE='ps2'),capture_output=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stderr.decode())
        self.assertIn(b'not opened',result.stdout)
        layered = self.run_cli('--mix',self.put('layers.acb',fx.acb(layers=2)),'-',19,0,self.root/'layers.wav')
        self.assertEqual(layered['frames'],(3904*48000+44099)//44100)

    def test_real_adx_decoder_on_bounded_voice_awb(self):
        bank = self.put('voice.awb',fx.awb([(77,fx.adx())]))
        decoded = self.run_cli('--voice-decode',bank,77,self.root/'voice.wav')
        self.assertEqual((decoded['frames'],decoded['rate'],decoded['channels']),(65,24000,1))
        mixed = self.run_cli('--voice-mix',bank,77,self.root/'voice-mixer.wav')
        self.assertEqual((mixed['frames'],mixed['rate'],mixed['channels']),(130,48000,2))
        self.run_cli('--voice-decode',bank,99,self.root/'missing.wav',good=False)
        bad = self.put('bad-voice.awb',fx.awb([(77,fx.adx()[:-1])]))
        self.run_cli('--voice-decode',bad,77,self.root/'bad-voice.wav',good=False)
        self.assertFalse((self.root/'bad-voice.wav').exists())

    def test_cue_sample_limit_and_loop_extension(self):
        short = self.put('short.acb',fx.acb(sample_limit=3804))
        decoded = self.run_cli('--cue-decode',short,'-',19,0,self.root/'short.wav')
        self.assertEqual(decoded['frames'],3804)
        mixed = self.run_cli('--mix',short,'-',19,0,self.root/'short-mixer.wav')
        self.assertEqual(mixed['frames'],(3804*48000+44099)//44100)
        self.run_cli('--mix',self.put('overlong.acb',fx.acb(sample_limit=5000)),
                     '-',19,0,self.root/'overlong.wav',good=False)
        loop = self.put('loop.acb',fx.acb(audio=fx.hca(loop=True),loop_range=(896,2944)))
        metadata = self.run_cli('--inspect',loop)['cues'][0]['waves'][0]
        self.assertEqual((metadata['loop_start'],metadata['loop_end']),(896,2944))
        result = self.run_cli('--mix-loop',loop,'-',19,0,self.root/'loop.wav')
        self.assertEqual(result['frames'],((2944+1024)*48000+44099)//44100)
        self.run_cli('--inspect',self.put('bad-loop.acb',fx.acb(loop_range=(3900,5000))),good=False)

    def test_hca_simd_window_alignment_regression(self):
        # FFmpeg7.1.1's unaligned HCA constant faults in i686 SIMD windowing.
        # Keep real decode calls on Win32, not just decoder/header discovery.
        path = self.put('stereo-window.hca',fx.hca(channels=2,blocks=8))
        out = self.root/'stereo-window.wav'
        result = self.run_cli('--decode',path,out)
        self.assertEqual(result['frames'],8*1024-128-64)
        self.assertEqual(result['channels'],2)
        with wave.open(str(out),'rb') as wav:
            self.assertEqual(wav.getnframes(),result['frames'])
            self.assertEqual(set(wav.readframes(result['frames'])),{0})

    def test_parallel_tracks_duplicate_occurrences_and_nested_synth(self):
        for name, data, count in (
                ('parallel',fx.acb(events=[fx.note()+fx.end()]*2),2),
                ('repeated',fx.acb(events=[fx.note()+fx.end()]*2,layers=2),4),
                ('nested',fx.acb(nested_synth=True,layers=2),2)):
            path = self.put(name+'.acb',data)
            cue = self.run_cli('--inspect',path)['cues'][0]
            self.assertTrue(cue['playback']['ok'])
            self.assertEqual(cue['playback']['count'],count)
            self.assertEqual([n['start_frame'] for n in cue['playback']['notes']],[0]*count)
            self.assertEqual(len(cue['waves']),count)
            mixed = self.run_cli('--mix',path,'-',19,1,self.root/(name+'.wav'))
            self.assertEqual(mixed['frames'],(3904*48000+44099)//44100)

    def test_relative_waits_overlap_end_children_and_max_end_bound(self):
        program = fx.note()+fx.wait(20)+fx.fine(0)+fx.wait(30)+fx.note()+fx.end()
        path = self.put('relative.acb',fx.acb(events=[program]))
        cue = self.run_cli('--inspect',path)['cues'][0]
        plan = cue['playback']
        self.assertTrue(plan['ok'])
        self.assertEqual([n['start_frame'] for n in plan['notes']],[0,50*48])
        expected = 50*48+(3904*48000+44099)//44100
        self.assertEqual(plan['frames'],expected)
        self.assertGreater(expected/48000,cue['length_ms']/1000)
        mixed = self.run_cli('--mix',path,'-',19,1,self.root/'relative.wav')
        self.assertEqual(mixed['frames'],expected)
        # A wait after the last note still keeps the event interpreter alive.
        tail = self.put('event-tail.acb',fx.acb(events=[fx.note()+fx.wait(200)+fx.end()]))
        self.assertEqual(self.run_cli('--mix',tail,'-',19,1,self.root/'tail.wav')['frames'],200*48)

    def test_empty_cue_and_late_zero_sample_child_preserve_completion(self):
        for name,data,frames in (
                ('empty',fx.acb(events=[],sequence_type=4,weights=[100]),0),
                ('zero',fx.acb(sample_limit=0,events=[fx.wait(10)+fx.note()+fx.end()]),480),
                ('wait-only',fx.acb(events=[fx.wait(10)+fx.end()]),480)):
            path = self.put(name+'.acb',data)
            plan = self.run_cli('--inspect',path)['cues'][0]['playback']
            self.assertTrue(plan['ok'])
            self.assertEqual(plan['frames'],frames)
            mixed = self.run_cli('--mix',path,'-',19,1,self.root/(name+'.wav'))
            self.assertEqual((mixed['frames'],mixed['rms']),(frames,0))

    def test_type3_inclusive_weight_boundaries_and_unweighted_choices(self):
        events = [fx.wait(i*10)+fx.note()+fx.end() for i in range(3)]
        path = self.put('random.acb',fx.acb(events=events,sequence_type=3,weights=[100]*3))
        refused = self.run_cli('--inspect',path)['cues'][0]['playback']
        self.assertFalse(refused['ok'])
        self.assertIn('A1:',refused['reason'])
        result = self.run_cli('--plan',path,19,0,100,101,200,201,300)
        self.assertEqual((result['last_upper'],result['draw_calls']),(300,6))
        self.assertEqual([p['notes'][0]['start_frame'] for p in result['plans']],[0,0,480,480,960,960])
        self.assertTrue(all(p['count']==1 for p in result['plans']))
        plain = self.put('unweighted.acb',fx.acb(events=events,sequence_type=3))
        result = self.run_cli('--plan',plain,19,0,1,2)
        self.assertEqual(result['last_upper'],2)
        self.assertEqual([p['notes'][0]['start_frame'] for p in result['plans']],[0,480,960])

    def test_type4_excludes_previous_track_and_keeps_sheet_row_state(self):
        events = [fx.wait(i*10)+fx.note()+fx.end() for i in range(3)]
        path = self.put('norepeat.acb',fx.acb(events=events,sequence_type=4,weights=[100]*3))
        result = self.run_cli('--plan',path,19,0,0,200,100)
        self.assertEqual((result['last_upper'],result['draw_calls']),(200,4))
        self.assertEqual([p['notes'][0]['start_frame'] for p in result['plans']],[0,480,960,0])
        bad = self.put('invalid-draw.acb',fx.acb(events=events,sequence_type=3,weights=[100]*3))
        self.assertIn('A1:',self.run_cli('--plan',bad,19,301)['plans'][0]['reason'])

    def test_unsupported_effects_types_and_malformed_events_refuse_explicitly(self):
        cases = [
            (fx.acb(sequence_type=1),'A2:'),
            (fx.acb(sequence_type=4,weights=[100]),'A2:'),
            (fx.acb(ratio=50),'A6:'),
            (fx.acb(pallet=0),'A3:'),
            (fx.acb(seq_command=fx.parameter(121,struct.pack('>h',-300))),'pending:M8d3-dsp:'),
            (fx.acb(track_commands=[fx.parameter(16,struct.pack('>H',10000))]),'pending:M8d3-dsp:'),
            (fx.acb(seq_command=fx.parameter(65,struct.pack('>I',9))),'A3:'),
            (fx.acb(seq_command=fx.parameter(89,struct.pack('>HH',100,1))),'pending:M8d3-dsp:'),
            (fx.acb(events=[fx.fine(-1)+fx.note()+fx.end()]),'A6:'),
            (fx.acb(events=[fx.fine(1)+fx.note()+fx.end()]),'A6:'),
            (fx.acb(events=[fx.note()]),'A8:'),
            (fx.acb(events=[fx.note()+fx.end()+fx.note()]),'A8:'),
            (fx.acb(events=[fx.parameter(2006,b'')+fx.end()]),'A8:'),
            (fx.acb(events=[struct.pack('>HB',2001,4)+b'\\0']),'A8:'),
        ]
        for i,(data,gap) in enumerate(cases):
            with self.subTest(i=i,gap=gap):
                path = self.put('refuse%d.acb'%i,data)
                plan = self.run_cli('--plan',path,19,0)['plans'][0]
                self.assertFalse(plan['ok'])
                self.assertIn(gap,plan['reason'])
                self.run_cli('--mix',path,'-',19,1,self.root/'refused.wav',good=False)
                self.assertFalse((self.root/'refused.wav').exists())
        skip = self.put('explicit-skip.acb',fx.acb(seq_command=fx.parameter(112,b'\\0'*4)))
        self.assertTrue(self.run_cli('--inspect',skip)['cues'][0]['playback']['ok'])
        # Fixture omission is an explicit port profile, not inferred retail authoring.
        omitted = self.put('port-profile.acb',fx.acb(omit_type=True))
        self.assertTrue(self.run_cli('--inspect',omitted)['cues'][0]['playback']['ok'])

    def test_gain_products_repeated_writes_and_explicit_zero(self):
        def gain(value):
            return fx.parameter(87,struct.pack('>H',value))
        data = fx.acb(seq_command=gain(50),track_commands=[gain(99)+gain(80)],synth_command=gain(25))
        path = self.put('gain.acb',data)
        plan = self.run_cli('--inspect',path)['cues'][0]['playback']
        self.assertTrue(plan['ok'])
        self.assertAlmostEqual(plan['notes'][0]['gain'],0.1,places=6)
        zero = self.put('zero-gain.acb',fx.acb(seq_command=gain(0),synth_command=gain(100)))
        self.assertEqual(self.run_cli('--inspect',zero)['cues'][0]['playback']['notes'][0]['gain'],0)
        fixed = self.put('fixed-gain.acb',fx.acb(seq_command=fx.parameter(89,struct.pack('>HH',86,0))))
        self.assertAlmostEqual(self.run_cli('--inspect',fixed)['cues'][0]['playback']['notes'][0]['gain'],0.86,places=6)

    def test_chronological_launch_order_and_same_time_authored_order(self):
        events = [fx.wait(50)+fx.note()+fx.end(),fx.note()+fx.wait(10)+fx.note()+fx.end()]
        path = self.put('ordered.acb',fx.acb(events=events))
        plan = self.run_cli('--inspect',path)['cues'][0]['playback']
        self.assertEqual([n['start_frame'] for n in plan['notes']],[0,480,2400])
        commands = [fx.parameter(87,struct.pack('>H',x)) for x in (50,80)]
        tied = self.put('tied.acb',fx.acb(events=[fx.wait(10)+fx.note()+fx.end()]*2,track_commands=commands))
        plan = self.run_cli('--inspect',tied)['cues'][0]['playback']
        for note,expected in zip(plan['notes'],(0.5,0.8)):
            self.assertAlmostEqual(note['gain'],expected,places=6)

    def test_unselected_alternative_is_validated_without_launching_it(self):
        path = self.put('alternative.acb',fx.acb(events=[fx.note()+fx.end()]*2,sequence_type=3,weights=[100,100],
                        track_commands=[None,fx.parameter(16,struct.pack('>H',10000))]))
        plan = self.run_cli('--plan',path,19,0)['plans'][0]
        self.assertFalse(plan['ok'])
        self.assertIn('pending:M8d3-dsp:',plan['reason'])

    def test_controller_blob_counts_and_unknown_schema_refuse(self):
        for fields,gap in (
                ([('LocalAisacs','blob',b'\\0\\1')],'pending:M8d3-dsp:'),
                ([('GlobalAisacNumRefs','u16',1)],'A3:'),
                ([('NumActionTracks','u16',1)],'A5:'),
                ([('NumInstPluginTracks','u16',1)],'A5:'),
                ([('LocalAisacNumRefs','u16',0)],'A7:'),
                ([('MysteryController','u8',0)],'A7:')):
            path = self.put('controller.acb',fx.acb(sequence_fields=fields))
            plan = self.run_cli('--plan',path,19,0)['plans'][0]
            self.assertFalse(plan['ok'])
            self.assertIn(gap,plan['reason'])
        self.run_cli('--acf-check',self.put('not-approved.acf',bytes(65536)),good=False)

    def test_schema_refusal_preserves_independent_missing_decoder_reason(self):
        fields=[('MysteryController','u8',0)]
        for codec,reason in ((2,'A7:'),(11,'CODEC11:')):
            path=self.put('schema-codec.acb',fx.acb(sequence_fields=fields,codec=codec))
            plan=self.run_cli('--plan',path,19,0)['plans'][0]
            self.assertFalse(plan['ok'])
            self.assertIn(reason,plan['reason'])

    def test_type4_previous_choice_is_independent_per_sequence_row(self):
        events = [fx.wait(i*10)+fx.note()+fx.end() for i in range(3)]
        path = self.put('rows.acb',fx.acb(events=events,sequence_type=4,weights=[100]*3,sequences=2))
        result = self.run_cli('--plan',path,19,'19:0','20:0','19:0','20:0')
        self.assertEqual([p['notes'][0]['start_frame'] for p in result['plans']],[0,0,480,480])

    def test_late_layer_corruption_removes_partial_mix_and_multi_loop_refuses(self):
        broken = bytearray(fx.hca(blocks=16)); broken[-1] ^= 1
        path = self.put('late-corruption.acb',fx.acb(audio=bytes(broken),layers=2,sample_limit=16000))
        self.run_cli('--mix',path,'-',19,1,self.root/'partial.wav',good=False)
        self.assertFalse((self.root/'partial.wav').exists())
        loop = self.put('multi-loop.acb',fx.acb(audio=fx.hca(loop=True),layers=2,loop_range=(896,2944)))
        plan = self.run_cli('--inspect',loop)['cues'][0]['playback']
        self.assertFalse(plan['ok'])
        self.assertIn('A5:',plan['reason'])


if __name__ == '__main__':
    unittest.main()
