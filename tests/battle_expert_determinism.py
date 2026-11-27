#!/usr/bin/env python3
"""Compare actual native decisions for two identically seeded expert runs."""
import argparse
import json
from pathlib import Path


def decisions(path, policy_id=9):
    path = Path(path)
    run = json.loads(path.read_text(encoding='utf-8'))
    if run['result']['outcome'] not in ('win', 'defeat') or run['result']['party_policy']['id'] != policy_id:
        raise ValueError('expected a completed policy-%d run' % policy_id)
    recorded = [a for a in run['attempts'] if a['status'] == 'recorded']
    if len(recorded) != 1:
        raise ValueError('expected one recorded native attempt')
    trace = path.parent / ('attempt-%d' % recorded[0]['attempt']) / 'trace.jsonl'
    sequence = []
    with trace.open(encoding='utf-8') as stream:
        for line in stream:
            event = json.loads(line)
            if event.get('kind') == 'policy_decision':
                chosen = event['candidates'][event['selected']['candidate']]
                sequence.append(('decision', event['units'][event['actor']]['id'], chosen['wheel'],
                                 chosen['skill'], chosen['item'], chosen['menu_index'], chosen['persona_slot'], event['selected']['target']))
            if event.get('kind') == 'command_commit' and event['actor']['present'] and event['actor']['side'] == 0:
                sequence.append(('command', event['actor']['id'], event['action_kind'], event['skill_id'],
                                 event['item_id'], event['target_count']))
            elif event.get('kind') == 'target_commit' and event['source']['present'] and event['source']['side'] == 0 and event['target']['present']:
                sequence.append(('target', event['source']['id'], event['skill_id'], event['target']['side'],
                                 event['target']['id'], event['target']['unit_uid']))
    if not sequence:
        raise ValueError('no recorded party decisions')
    return run['seeds'], run['identity']['entry_sha256'], sequence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('first', type=Path)
    parser.add_argument('second', type=Path)
    args = parser.parse_args()
    a, b = decisions(args.first), decisions(args.second)
    if a != b:
        raise ValueError('seed, entry, or native decision sequence differs')
    print('identical expert decision sequence:', len(a[2]), 'decision/command/target records')


if __name__ == '__main__':
    main()
