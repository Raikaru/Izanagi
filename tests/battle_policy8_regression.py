#!/usr/bin/env python3
"""Compare native policy-8 decisions before and after the snapshot cutover."""
import argparse
from pathlib import Path
from battle_expert_determinism import decisions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before', type=Path)
    parser.add_argument('after', type=Path)
    args = parser.parse_args()
    first, second = decisions(args.before, 8), decisions(args.after, 8)
    if first != second:
        raise ValueError('seed, entry, or strategic native decision sequence differs')
    print('unchanged strategic decisions:', len(first[2]), 'command/target records')


if __name__ == '__main__':
    main()
