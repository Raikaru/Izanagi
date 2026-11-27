#!/usr/bin/env python3
"""Find and assert a pinned native expert decision state in a retained trace."""
import argparse
import json
from pathlib import Path

DAMAGE = {1, 4, 8, 10, 12, 13, 14}


def fixed_heal(state, _absorbed):
    if any(u['side'] == 1 and u['charged'] for u in state['units']):
        return None  # Main's charge rule makes Guard higher priority.
    hero = next((i for i, u in enumerate(state['units']) if u['side'] == 0 and u['id'] == 1 and u['hp'] == 57 and u['max_hp'] == 329), None)
    if hero is None:
        return None
    items = {c['item']: c for c in state['candidates'] if c['item'] in (769, 771) and c['targets'] & (1 << hero)}
    if set(items) != {769, 771} or (items[769]['hp_effect'], items[769]['power'], items[771]['hp_effect'], items[771]['power']) != (5, 20, 5, 100):
        return None
    selected = state['candidates'][state['selected']['candidate']]
    if selected['item'] != 771 or state['selected']['target'] != hero:
        raise ValueError('expert did not choose the 100-HP item for the hero')
    return {'hero_index': hero, 'selected_item': selected['item']}


def persona_discipline(state, _absorbed):
    if state['units'][state['actor']]['id'] != 1:
        return None
    personas = {p['slot']: p for p in state['personas']}
    lead = personas.get(state['lead_slot'])
    if lead is None:
        return None
    def lower(c):
        p = personas.get(c['persona_slot'])
        if p is None or c['hp_effect'] not in DAMAGE:
            return False
        indices = (0,) if c['element'] == 0 else (1,) if c['element'] > 0 else (0, 1)
        return any(p['stats'][i] < lead['stats'][i] for i in indices)
    alternatives = [c for c in state['candidates'] if c['persona_slot'] != 255 and lower(c)]
    if not alternatives:
        return None
    selected = state['candidates'][state['selected']['candidate']]
    if selected['persona_slot'] != 255 and lower(selected):
        raise ValueError('expert selected a lower-stat damaging Persona')
    return {'lower_stat_candidates': len(alternatives), 'selected_slot': selected['persona_slot']}


def weakness_evidence(state, absorbed):
    candidates = state['candidates']
    for i, unit in enumerate(state['units']):
        if unit['side'] != 1:
            continue
        eligible = [c for c in candidates if c['persona_slot'] == 255 and c['hp_effect'] in DAMAGE and
                    c['targets'] & (1 << i) and 0 <= c['element'] < 32]
        weak = [c for c in eligible if unit['weak'] & (1 << c['element'])]
        bad = [c for c in eligible if unit['bad'] & (1 << c['element']) and (unit['uid'], c['skill']) in absorbed]
        untried = [c for c in eligible if not unit['tried'] & (1 << c['element'])]
        if not (weak and bad and untried):
            continue
        selected = candidates[state['selected']['candidate']]
        if state['selected']['target'] != i or selected['element'] < 0 or selected['element'] >= 32 or not unit['weak'] & (1 << selected['element']):
            raise ValueError('expert did not exploit the observed weakness on the pinned target')
        return {'target_index': i, 'weak_element': selected['element']}
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', required=True, type=Path)
    parser.add_argument('--kind', required=True, choices=('fixed-heal', 'persona-switch', 'weakness-evidence'))
    args = parser.parse_args()
    run = json.loads(args.run.read_text(encoding='utf-8'))
    if run['result']['outcome'] not in ('win', 'defeat') or run['result']['party_policy']['id'] != 9:
        raise ValueError('expected a completed expert-policy native run')
    attempts = [a for a in run['attempts'] if a['status'] == 'recorded']
    if len(attempts) != 1:
        raise ValueError('expected one recorded native attempt')
    trace = args.run.parent / ('attempt-%d' % attempts[0]['attempt']) / 'trace.jsonl'
    assertion = {'fixed-heal': fixed_heal, 'persona-switch': persona_discipline,
                 'weakness-evidence': weakness_evidence}[args.kind]
    absorbed = set()
    with trace.open(encoding='utf-8') as stream:
        for line in stream:
            event = json.loads(line)
            if event.get('kind') == 'target_commit' and event['source']['present'] and event['source']['side'] == 0 and event['target']['present'] and event['target']['side'] == 1 and event['resources_applied'] and event['hp_after'] > event['hp_before']:
                absorbed.add((event['target']['unit_uid'], event['skill_id']))
            if event.get('kind') != 'policy_decision' or event['policy'] != 9:
                continue
            evidence = assertion(event, absorbed)
            if evidence is not None:
                print(json.dumps({'kind': args.kind, 'update': event['update'], 'evidence': evidence}, sort_keys=True))
                return
    raise ValueError('no qualifying native decision state occurred')


if __name__ == '__main__':
    main()
