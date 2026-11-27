#!/usr/bin/env python3
"""Owner-local Steam audio qualification. All game bytes/WAVs stay on this PC."""
import array
import collections
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/golden_mode'))
import census_scripts
import formats
import inventory


def main(command):
    install = os.environ.get('P4PC_GOLDEN_STEAM')
    manifest = os.environ.get('P4PC_GOLDEN_INVENTORY')
    output = os.environ.get('P4PC_GOLDEN_AUDIO_OUT')
    if not all((install,manifest,output)):
        print('SKIP: local Steam inventory/audio output not configured')
        return 77
    if os.name != 'nt':
        raise RuntimeError('Steam audio qualification is owner-PC only; never transfer assets to a lane')
    install = Path(install).resolve()
    output = Path(output).resolve()
    if output.is_relative_to(ROOT) or output.is_relative_to(install):
        raise RuntimeError('audio evidence must be outside source and install')
    census_scripts.verify_steam(install)
    output.mkdir(parents=True,exist_ok=True)
    rows = [json.loads(line) for line in Path(manifest).open(encoding='utf8')]
    selected = [r for r in rows if r['path'].lower().endswith(('.acb','.awb'))]
    by_path = {r['path']:r for r in rows}
    cache = {}
    def extract(path):
        chain = path.split('::')
        if len(chain)==1:
            data=(install/chain[0]).read_bytes()
        else:
            if chain[0] not in cache:
                handle = (install/chain[0]).open('rb')
                cache[chain[0]] = (handle,{n:(o,s,e) for n,o,s,e in formats.cpk_entries(formats.file_reader(handle))})
            handle,entries = cache[chain[0]]
            off,stored,expanded = entries[chain[1]]
            handle.seek(off); data = handle.read(stored)
            if len(data)!=stored: raise ValueError('truncated source')
            if stored != expanded: data = formats.crilayla(data)
            for name in chain[2:]:
                _,entries = inventory.children(data)
                off,size = next((o,s) for n,o,s in entries or () if n==name)
                data = data[off:off+size]
                if data[:8]==b'CRILAYLA': data=formats.crilayla(data)
        if hashlib.sha256(data).hexdigest()!=by_path[path]['sha256']:
            raise ValueError('source SHA mismatch: '+path)
        return data
    env = dict(os.environ,P4PC_CONFIG='none',P4PC_MODE='golden',P4PC_INPUT='script')
    counts=collections.Counter(); tables=collections.Counter(); candidates=[]; errors=[]
    wave_counts=collections.Counter(); cue_graphs=[]; playback_outcomes=collections.Counter(); named_refusals=collections.Counter()
    acf_paths=('data.cpk::sound/adx2/common.acf','data_cri/sound/adx2/common.acf')
    acf_bytes={path:extract(path) for path in acf_paths}
    with (output/'census-driver.log').open('wb') as log:
        process=subprocess.Popen(command+['--batch-qualify'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=log,env=env)
        def request(kind,data):
            process.stdin.write(kind+struct.pack('<I',len(data))+data)
            process.stdin.flush()
            try: return json.loads(process.stdout.readline())
            except (json.JSONDecodeError,UnicodeDecodeError): return {'error':'invalid native response'}
        try:
            if request(b'F',acf_bytes[acf_paths[0]])!={'acf_profile':1}:
                raise AssertionError('registered packed ACF qualification failed')
            for row in selected:
                data=extract(row['path']); awb=row['path'].lower().endswith('.awb')
                if not awb:
                    paired=row['path'][:-4]+'.awb'
                    bank=request(b'B',extract(paired) if paired in by_path else b'')
                    if 'error' in bank: raise RuntimeError('native paired bank refused: '+paired)
                result=request(b'W' if awb else b'A',data)
                if 'error' in result:
                    errors.append({'path':row['path'],'reason':result['error']});continue
                counts['awb' if awb else 'acb']+=1
                counts['external_waves' if awb else 'memory_waves']+=result['bank_waves']
                counts['external_hca' if awb else 'memory_hca']+=result['hca']
                counts['other_waveforms']+=result['other']
                if not awb:
                    for name,n in zip(('cue','name','waveform','synth','sequence','track','event','command','extension',
                                       'seq_command','track_command','synth_command'),result['tables']): tables[name]+=n
                    for cue in result['cues']:
                        counts['resolved_cues']+=1
                        counts['resolved_edges']+=len(cue['waves'])
                        wave_counts[len(cue['waves'])]+=1
                        cue_graphs.append({'path':row['path'],**cue,'wave_count':len(cue['waves'])})
                        playback=cue['playback']
                        if playback['ok']:
                            if not cue.get('start',{}).get('ok'):
                                errors.append({'path':row['path'],'cue_id':cue['id'],
                                               'reason':'closed plan failed native start: '+str(cue.get('start'))})
                            playback_outcomes['empty' if not playback['count'] and not playback['frames'] else 'playable_start_proven']+=1
                        else:
                            if not playback.get('reason'): raise AssertionError('unnamed cue refusal')
                            label='pending:M8d3-dsp' if playback['reason'].startswith('pending:M8d3-dsp:') else playback['reason'].split(':',1)[0]
                            playback_outcomes['pending:M8d3-dsp' if label=='pending:M8d3-dsp' else 'refused_named_spec_gap']+=1
                            named_refusals[label]+=1
                        if len(cue['waves'])==1 and cue['waves'][0]['codec']==2 and cue['length_ms'] != 0:
                            candidates.append({'path':row['path'],'cue':cue})
                if counts['acb']%100==0 and not awb: print('parsed ACB',counts['acb'],flush=True)
        finally:
            process.stdin.close()
            rc=process.wait(timeout=60)
    receipt={'source_build':10836033,'inventory_sha256':hashlib.sha256(Path(manifest).read_bytes()).hexdigest(),
             'objects':len(selected),'counts':dict(counts),'tables':dict(tables),'errors':errors,
             'cue_wave_counts':dict(sorted(wave_counts.items())),'cue_graphs':cue_graphs,
             'playback_outcomes':dict(playback_outcomes),'named_refusals':dict(named_refusals),'witnesses':[]}
    receipt['acf_sources']=[{'path':path,'sha256':by_path[path]['sha256']} for path in acf_paths]
    receipt['preconditions']={'cri_player_category_controls':'unity','wet_dsp_state':'initially clear',
                              'category_admission':'idle, producer-serialized start/destroy',
                              'single_wave_loop_policy':'existing explicit one-shot caller policy',
                              'multi_wave_loops':'A5 refusal'}
    (output/'audio-receipt.json').write_text(json.dumps(receipt,indent=2))
    if rc or errors: raise RuntimeError('native census errors: '+str(errors[:10]))
    expected_wave_counts={0:707,1:2563,2:183,3:123,4:25,5:23,6:2,7:2,8:2,11:1}
    if dict(wave_counts)!=expected_wave_counts:
        raise AssertionError(('cue graph wave census',dict(wave_counts),expected_wave_counts))
    if len(cue_graphs)!=3631:
        raise AssertionError(('missing cue graph records',len(cue_graphs)))
    expected={'acb':630,'awb':1100,'memory_hca':2322,'external_hca':1064,'other_waveforms':50025,'resolved_cues':3631}
    for key,value in expected.items():
        if counts[key]!=value: raise AssertionError((key,counts[key],value))
    expected_tables={'cue':3631,'name':3631,'waveform':3425,'synth':3605,'sequence':3592,'track':3462,
                     'event':3462,'command':0,'extension':165,'seq_command':3592,'track_command':3424,'synth_command':3164}
    if dict(tables)!=expected_tables: raise AssertionError((dict(tables),expected_tables))
    chosen=[]
    for category,words in ((0,('::sound/adx2/bgm/',)),(1,('::sound/adx2/comse/',))):
        hits=[c for c in candidates if any(word in c['path'].lower() for word in words)
              and c['cue']['waves'][0]['samples']/c['cue']['waves'][0]['rate'] > (10 if category == 0 else 0.15)
              and (category != 0 or (c['cue']['waves'][0]['loop'] and c['cue']['waves'][0]['channels'] == 2
                                     and c['cue']['waves'][0]['rate'] == 48000))]
        if not hits: raise RuntimeError('no qualifying Steam cue for category '+str(category))
        # Prefer a non-looping cue, with the shortest metadata duration.
        hit=min(hits,key=lambda c:(bool(c['cue']['waves'][0]['loop']),c['cue']['length_ms']))
        chosen.append((category,hit))
    with tempfile.TemporaryDirectory(prefix='raw-inputs-',dir=output) as temporary:
        temp=Path(temporary)
        acf_files={}
        for index,path in enumerate(acf_paths):
            file=temp/('config%d.acf'%index); file.write_bytes(acf_bytes[path]); acf_files[path]=str(file)
            checked=subprocess.run(command+['--acf-check',str(file)],capture_output=True,env=env,timeout=600)
            if checked.returncode or json.loads(checked.stdout)!={'acf_profile':index+1}:
                raise AssertionError('measured ACF did not qualify: '+path)
        primary_acf=acf_files[acf_paths[0]]
        for category,hit in chosen:
            source=extract(hit['path']); acb=temp/'cue.acb';acb.write_bytes(source)
            cue=hit['cue']; w=cue['waves'][0]; bank_path='-'; bank_sha=None
            if w['streaming']:
                paired=hit['path'][:-4]+'.awb'
                if paired not in by_path: raise RuntimeError('paired AWB absent: '+paired)
                bank=temp/'cue.awb';raw=extract(paired);bank.write_bytes(raw);bank_path=str(bank);bank_sha=hashlib.sha256(raw).hexdigest()
            witnesses={}
            for operation in ('--cue-decode','--mix'):
                name=('decode' if operation=='--cue-decode' else 'mixer')+'-'+str(category)+'.wav'
                wav_path=output/name
                run=subprocess.run(command+[operation,str(acb),bank_path,str(cue['id']),str(category),str(wav_path),primary_acf],
                                   capture_output=True,env=env,timeout=600)
                if operation=='--mix' and not cue['playback']['ok']:
                    gap='pending:M8d3-dsp' if cue['playback']['reason'].startswith('pending:M8d3-dsp:') else cue['playback']['reason'].split(':',1)[0]
                    if not run.returncode or wav_path.exists() or gap not in run.stderr.decode(errors='replace'):
                        raise AssertionError('unapproved full cue did not refuse under its named gap')
                    witnesses[operation]={'refused':True,'named_refusal':cue['playback']['reason']}
                    continue
                if run.returncode: raise RuntimeError(run.stderr.decode(errors='replace'))
                result=json.loads(run.stdout)
                if result['rms']<=1 or not math.isfinite(result['rms']): raise AssertionError('silent/nonfinite Steam decode')
                expected_seconds=w['samples']/w['rate'] if operation=='--cue-decode' else cue['playback']['frames']/48000
                if abs(result['seconds']-expected_seconds)>1/result['rate']+1e-6: raise AssertionError('waveform/plan duration mismatch')
                with wave.open(str(wav_path),'rb') as wav:
                    if wav.getnframes()!=result['frames'] or wav.getframerate()!=result['rate']: raise AssertionError('WAV header mismatch')
                result.update(wav=name,sha256=hashlib.sha256(wav_path.read_bytes()).hexdigest())
                witnesses[operation]=result
            if category == 0 and cue['playback']['ok']:
                # Compare actual post-wrap samples against the same stream's
                # first pass. At 48kHz there is no resampling phase ambiguity.
                name='loop-mixer-0.wav'; wav_path=output/name
                run=subprocess.run(command+['--mix-loop',str(acb),bank_path,str(cue['id']),str(category),str(wav_path),primary_acf],
                                   capture_output=True,env=env,timeout=600)
                if run.returncode: raise RuntimeError(run.stderr.decode(errors='replace'))
                loop_result=json.loads(run.stdout)
                offset=cue['playback']['notes'][0]['start_frame']
                if loop_result['frames'] != offset+w['loop_end']+1024 or loop_result['rms'] <= 1:
                    raise AssertionError('loop capture duration or energy mismatch')
                with wave.open(str(output/witnesses['--mix']['wav']),'rb') as normal:
                    normal.setpos(offset+w['loop_start']); expected=array.array('h',normal.readframes(1024))
                with wave.open(str(wav_path),'rb') as looped:
                    looped.setpos(offset+w['loop_end']); actual=array.array('h',looped.readframes(1024))
                if len(expected)!=2048 or len(actual)!=2048: raise AssertionError('short loop comparison window')
                peak=max(abs(a-b) for a,b in zip(expected,actual))
                if peak>2: raise AssertionError('post-loop PCM differs from first pass: '+str(peak))
                loop_result.update(wav=name,sha256=hashlib.sha256(wav_path.read_bytes()).hexdigest(),
                                   compared_frames=1024,max_abs_difference=peak)
                witnesses['--mix-loop']=loop_result
            elif category == 0:
                wav_path=output/'loop-mixer-0.wav'
                run=subprocess.run(command+['--mix-loop',str(acb),bank_path,str(cue['id']),str(category),str(wav_path),primary_acf],
                                   capture_output=True,env=env,timeout=600)
                gap='pending:M8d3-dsp' if cue['playback']['reason'].startswith('pending:M8d3-dsp:') else cue['playback']['reason'].split(':',1)[0]
                if not run.returncode or wav_path.exists() or gap not in run.stderr.decode(errors='replace'):
                    raise AssertionError('unapproved BGM loop did not retain its explicit refusal')
                witnesses['--mix-loop']={'refused':True,'named_refusal':cue['playback']['reason']}
            receipt['witnesses'].append({'category':category,'source':hit['path'],'source_sha256':hashlib.sha256(source).hexdigest(),
                                         'bank_sha256':bank_sha,'cue':cue,'outputs':witnesses})
        receipt['multiwave_witnesses']=[]
        for source_path,cue_id in (('data.cpk::skill/07D.pac::07D.acb',1),('data.cpk::skill/06F.pac::06F.acb',0)):
            hit=next(g for g in cue_graphs if g['path']==source_path and g['id']==cue_id)
            plan=hit['playback']
            if not plan['ok'] or plan['count']<2 or not hit.get('start',{}).get('ok'):
                raise AssertionError(('approved multi-wave graph is not playable',source_path,plan,hit.get('start')))
            by_index={w['index']:w for w in hit['waves']}
            ends=[note['start_frame']+(by_index[note['index']]['samples']*48000+by_index[note['index']]['rate']-1)//by_index[note['index']]['rate']
                  for note in plan['notes']]
            if plan['frames']!=max(ends): raise AssertionError('multi-wave max-end bound mismatch')
            if cue_id==1:
                expected=[(0,0,.78),(3216,1,.76),(9600,0,.78),(13632,1,.78)]
                if plan['count']!=4 or plan['frames']!=30197: raise AssertionError('four-note witness count/bound mismatch')
                for note,(start,index,gain) in zip(plan['notes'],expected):
                    if note['start_frame']!=start or note['index']!=index or abs(note['gain']-gain)>1e-6:
                        raise AssertionError(('authored four-note scheduling/gain mismatch',note))
            elif [n['start_frame'] for n in plan['notes']]!=[0,2503*48]:
                raise AssertionError('relative two-note witness timing mismatch')
            acb=temp/'multi.acb'; acb.write_bytes(extract(source_path))
            bank_path='-'
            if any(w['streaming'] for w in hit['waves']):
                bank=temp/'multi.awb'; bank.write_bytes(extract(source_path[:-4]+'.awb')); bank_path=str(bank)
            outputs={}
            variants=acf_paths if cue_id==1 else acf_paths[:1]
            for index,acf_path in enumerate(variants):
                name='multi-'+Path(source_path.split('::')[1]).stem+'-'+str(cue_id)+'-acf'+str(index)+'.wav'
                wav_path=output/name
                run=subprocess.run(command+['--mix',str(acb),bank_path,str(cue_id),'1',str(wav_path),acf_files[acf_path]],
                                   capture_output=True,env=env,timeout=600)
                if run.returncode: raise RuntimeError(run.stderr.decode(errors='replace'))
                pcm=json.loads(run.stdout)
                if pcm['frames']!=plan['frames'] or pcm['rate']!=48000 or pcm['channels']!=2:
                    raise AssertionError('multi-wave WAV duration/format differs from its plan')
                if pcm['rms']<=1 or not math.isfinite(pcm['rms']): raise AssertionError('silent/nonfinite multi-wave cue')
                with wave.open(str(wav_path),'rb') as wav:
                    if (wav.getnframes(),wav.getframerate(),wav.getnchannels())!=(plan['frames'],48000,2):
                        raise AssertionError('multi-wave WAV header mismatch')
                outputs[acf_path]={**pcm,'wav':name,'sha256':hashlib.sha256(wav_path.read_bytes()).hexdigest()}
            if len(outputs)==2 and len({r['sha256'] for r in outputs.values()})!=1:
                raise AssertionError('common dry ACF variants produced different PCM')
            receipt['multiwave_witnesses'].append({'source':source_path,'source_sha256':by_path[source_path]['sha256'],
                                                   'cue_id':cue_id,'plan':plan,'outputs':outputs})
        # The measured explicit voice banks contain ADX, not HCA and no ACB.
        # Exercise a real English voice entry through the native AWB reader
        # and the already-shipped unchanged ADX decoder, as Main ruled.
        voice_path = next(r['path'] for r in selected
                          if r['path'].lower().startswith('data.cpk::sound/adx2/en/v')
                          and r['path'].lower().endswith('.awb'))
        raw = extract(voice_path)
        bank = temp/'voice.awb'; bank.write_bytes(raw)
        voice_id = None
        for name,off,size in inventory.afs2_entries(raw) or ():
            entry = raw[off:off+size]
            if len(entry) < 20 or entry[:2] != b'\x80\0': continue
            rate,frames = struct.unpack_from('>II',entry,8)
            header = struct.unpack_from('>H',entry,2)[0]+4
            # Select a residual-bearing stream for the non-silent witness;
            # do not relabel silent data or relax the output RMS check.
            end = min(len(entry),header+((frames+31)//32)*18*entry[7])
            active = any(any(entry[p+2:p+18]) for p in range(header,end-17,18))
            if rate and frames/rate > 0.5 and active:
                voice_id = int(name)
                voice_rate,voice_frames = rate,frames
                break
        if voice_id is None: raise RuntimeError('no finite voice entry in measured English AWB')
        witnesses={}
        for operation in ('--voice-decode','--voice-mix'):
            name=('decode' if operation=='--voice-decode' else 'mixer')+'-2.wav'
            wav_path=output/name
            run=subprocess.run(command+[operation,str(bank),str(voice_id),str(wav_path)],
                               capture_output=True,env=env,timeout=600)
            if run.returncode: raise RuntimeError(run.stderr.decode(errors='replace'))
            result=json.loads(run.stdout)
            if result['rms']<=1 or not math.isfinite(result['rms']): raise AssertionError('silent/nonfinite voice')
            if result['original_frames'] != voice_frames or abs(result['seconds']-voice_frames/voice_rate)>1/result['rate']+1e-6:
                raise AssertionError('ADX voice header duration mismatch')
            with wave.open(str(wav_path),'rb') as wav:
                if wav.getnframes()!=result['frames'] or wav.getframerate()!=result['rate']: raise AssertionError('voice WAV header mismatch')
            result.update(wav=name,sha256=hashlib.sha256(wav_path.read_bytes()).hexdigest())
            witnesses[operation]=result
        receipt['witnesses'].append({'category':2,'codec':'adx','source':voice_path,'wave_id':voice_id,
                                     'source_sha256':hashlib.sha256(raw).hexdigest(),'frames':voice_frames,
                                     'rate':voice_rate,'outputs':witnesses})
    for handle,_ in cache.values():handle.close()
    (output/'audio-receipt.json').write_text(json.dumps(receipt,indent=2))
    print(json.dumps({'counts':dict(counts),'tables':dict(tables),'witnesses':len(receipt['witnesses'])}),flush=True)
    return 0


if __name__=='__main__':
    raise SystemExit(main(sys.argv[1:]))
