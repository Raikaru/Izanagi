#!/usr/bin/env python3
"""Assert that the expert policy matches policy 8 through a pinned recovery decision.

Compares two retained native battle_sim runs of the same seed: the expert's party
commands must be identical to policy 8's from --from-admission (default: the
start) through the named ordinary-queue admission, and the last of them must not
be Guard. Never synthesizes a battle.
"""
import argparse
import json
from pathlib import Path

GUARD = 7


def party_commands(path):
    path = Path(path)
    run = json.loads((path / 'run.json').read_text(encoding='utf-8'))
    recorded = [a for a in run['attempts'] if a['status'] == 'recorded']
    if len(recorded) != 1:
        raise ValueError('expected exactly one recorded attempt')
    admission, commands = 0, []
    with (path / ('attempt-%d' % recorded[0]['attempt']) / 'trace.jsonl').open(encoding='utf-8') as stream:
        for line in stream:
            if '"kind":"update"' in line or '"kind":"entry"' in line:
                continue
            event = json.loads(line)
            if event.get('kind') == 'admission':
                admission += 1
            elif event.get('kind') == 'command_commit' and event['actor'].get('side') == 0:
                commands.append((admission, event['actor']['id'], event['action_kind'],
                                 event['skill_id'], event['item_id']))
    return run, commands


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--expert', required=True, type=Path, help='run-NNN directory of the expert policy')
    parser.add_argument('--strategic', required=True, type=Path, help='run-NNN directory of policy 8, same seed')
    parser.add_argument('--admission', required=True, type=int)
    parser.add_argument('--from-admission', type=int, default=0, help='first admission compared; earlier decisions may differ')
    parser.add_argument('--expect-outcome', default='win', help='terminal outcome, or any')
    args = parser.parse_args()
    expert, expert_commands = party_commands(args.expert)
    strategic, strategic_commands = party_commands(args.strategic)
    if expert['seeds'] != strategic['seeds'] or expert['identity']['entry_sha256'] != strategic['identity']['entry_sha256']:
        raise ValueError('runs do not share seeds and entry')
    if expert['result']['party_policy']['id'] != 9 or strategic['result']['party_policy']['id'] != 8:
        raise ValueError('expected expert 9 and strategic 8')
    prefix = [c for c in strategic_commands if args.from_admission <= c[0] <= args.admission]
    if not prefix or prefix[-1][0] != args.admission:
        raise ValueError('policy 8 has no party command at the named admission')
    if [c for c in expert_commands if args.from_admission <= c[0] <= args.admission] != prefix:
        raise ValueError('expert commands differ from policy 8 through the named admission')
    if prefix[-1][2] == GUARD:
        raise ValueError('the pinned decision is Guard, not recovery')
    if args.expect_outcome != 'any' and expert['result']['outcome'] != args.expect_outcome:
        raise ValueError('expert outcome %s, expected %s' % (expert['result']['outcome'], args.expect_outcome))
    print(json.dumps({'seeds': expert['seeds'], 'admission': args.admission, 'command': prefix[-1],
                      'outcome': expert['result']['outcome']}, sort_keys=True))


if __name__ == '__main__':
    main()
