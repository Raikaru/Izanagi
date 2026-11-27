#!/usr/bin/env python3
"""Assert a native expert opening witness and optionally compare another worker.

Requires retained battle_sim run.json and its trace; never synthesizes a battle.
"""
import argparse
import json
from pathlib import Path


def decision(path, expected):
    path = Path(path)
    run = json.loads(path.read_text(encoding='utf-8'))
    if run['run_index'] != expected['run_index']:
        raise ValueError('run index differs from the pinned witness')
    if run['result']['outcome'] not in ('win', 'defeat'):
        raise ValueError('native run did not reach a win or defeat')
    if run['result']['party_policy']['id'] != 9:
        raise ValueError('witness must use expert policy 9')
    recorded = [a for a in run['attempts'] if a['status'] == 'recorded']
    if len(recorded) != 1:
        raise ValueError('expected exactly one recorded attempt')
    trace = path.parent / ('attempt-%d' % recorded[0]['attempt']) / 'trace.jsonl'
    commands = 0
    hits = []
    charged = False
    party = []
    decision_state = None
    with trace.open(encoding='utf-8') as stream:
        for line in stream:
            event = json.loads(line)
            if event.get('kind') == 'policy_decision':
                decision_state = event
            if event.get('kind') in ('entry', 'update'):
                party = event['party']
            if event.get('kind') == 'command_commit' and event['actor']['present'] and event['actor']['side'] == 1:
                charged = False
            if event.get('kind') == 'target_commit' and event['source']['present'] and event['source']['side'] == 1 and event['target']['present'] and event['target']['side'] == 1 and event['buff_changes'] & 0xc00:
                charged = True
            if event.get('kind') == 'target_commit' and event['source']['present'] and event['source']['side'] == 0 and event['target']['present'] and event['target']['side'] == 1:
                hits.append(event)
            if event.get('kind') != 'command_commit' or not event['actor']['present'] or event['actor']['side'] != 0:
                continue
            if commands == expected['opening_party_commands']:
                if event['actor']['id'] != expected['next_actor_id']:
                    raise ValueError('opening did not hand off at the expected actor')
                if decision_state is None or decision_state['policy'] != 9:
                    raise ValueError('no expert observation was recorded at the handoff')
                actor = decision_state['units'][decision_state['actor']]
                if actor['id'] != expected['next_actor_id']:
                    raise ValueError('observed actor differs at handoff')
                selected = decision_state['candidates'][decision_state['selected']['candidate']]
                if expected.get('requires_observed_surviving_enemy_hit') and not any(h['resources_applied'] and h['hp_after'] > 0 and h['hp_after'] < h['hp_before'] for h in hits):
                    raise ValueError('no observed surviving enemy hit before handoff')
                if expected.get('requires_no_observed_weakness') and any(h['target_flags'] & 4 for h in hits):
                    raise ValueError('a weakness was observed before handoff')
                if expected.get('requires_no_observed_weakness') and any(u['weak'] for u in decision_state['units'] if u['side'] == 1):
                    raise ValueError('a weakness is already known in the observation')
                if expected.get('requires_visible_charge') and not charged:
                    raise ValueError('no unconsumed enemy charge was observed before handoff')
                if expected.get('requires_visible_charge') and not any(u['charged'] for u in decision_state['units'] if u['side'] == 1):
                    raise ValueError('the handoff observation does not show a charge')
                if expected.get('requires_needy_ally') and not any(u['hp'] and u['hp'] * 100 <= u['max_hp'] * (70 if u['id'] == 1 else 50) for u in party):
                    raise ValueError('no living ally was below the healing threshold')
                if expected.get('requires_needy_ally') and not any(u['hp'] and u['hp'] * 100 <= u['max_hp'] * (70 if u['id'] == 1 else 50) for u in decision_state['units'] if u['side'] == 0):
                    raise ValueError('the handoff observation does not show a needy ally')
                if 'expected_action_kind' in expected and event['action_kind'] != expected['expected_action_kind']:
                    raise ValueError('expert did not choose the pinned action')
                if expected.get('requires_visible_charge') and selected['wheel'] != 2:
                    raise ValueError('expert did not select Guard in the observation')
                if 'preferred_setup_skill_ids' in expected and event['skill_id'] not in expected['preferred_setup_skill_ids']:
                    raise ValueError('expert did not choose a pinned setup skill')
                if 'preferred_setup_skill_ids' in expected and (selected['persona_slot'] != 255 or selected['skill'] not in expected['preferred_setup_skill_ids']):
                    raise ValueError('expert did not select the pinned setup skill')
                return {'seeds': run['seeds'], 'entry_sha256': run['identity']['entry_sha256'],
                        'actor_id': event['actor']['id'], 'action_kind': event['action_kind'],
                        'skill_id': event['skill_id'], 'item_id': event['item_id']}
            commands += 1
    raise ValueError('no named-policy command after the scripted opening')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', required=True, type=Path)
    parser.add_argument('--expect', required=True, type=Path)
    parser.add_argument('--against', type=Path)
    args = parser.parse_args()
    expected = json.loads(args.expect.read_text(encoding='utf-8'))
    observed = decision(args.run, expected)
    if args.against is not None and observed != decision(args.against, expected):
        raise ValueError('native decision differs between the paired runs')
    print(json.dumps(observed, sort_keys=True))


if __name__ == '__main__':
    main()
