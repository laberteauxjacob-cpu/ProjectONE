"""Tiny offline policy/identity fixtures; never read actual captured media."""
import argparse
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import wave

SPEC = importlib.util.spec_from_file_location('evidence_assembly', Path(__file__).with_name('assemble_candidate04_evidence.py'))
ASSEMBLY = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ASSEMBLY)
LABELS = {'0': 'BEFORE', '83': ASSEMBLY.FINAL_IDLE_LABEL}


def rows_for(times):
    return [{'file': 'frame_%05d.jpg' % index, 'audio_seconds': str(t), 'world_seconds': str(t+2),
             'phase': '0' if index == 0 else '83'} for index, t in enumerate(times)]


class TerminalPolicyTests(unittest.TestCase):
    def test_strict_default_and_opt_in_single_late(self):
        times = [0., .6, 1.5, 2.5, 3.5, 3.98, 4.005]
        with self.assertRaises(ValueError): ASSEMBLY.terminal_idle_policy(rows_for(times), times, 4., LABELS)
        kept, ledger = ASSEMBLY.terminal_idle_policy(rows_for(times), times, 4., LABELS, True)
        self.assertEqual(kept, 6)
        self.assertEqual(ledger['original_captured_frames'], 7)
        self.assertEqual(ledger['omitted_callbacks'][0]['file'], 'frame_00006.jpg')
        self.assertAlmostEqual(ledger['source_tail_gap_seconds'], .005)
        self.assertAlmostEqual(ledger['final_presented_frame_hold_seconds'], .02)

    def test_inclusive_boundary_and_three_callback_limit(self):
        times = [0., .6, 1.5, 2.5, 3.9, 4., 4.05, 4.1]
        kept, ledger = ASSEMBLY.terminal_idle_policy(rows_for(times), times, 4., LABELS, True)
        self.assertEqual(kept, 5)
        self.assertEqual(len(ledger['omitted_callbacks']), 3)
        for invalid in (times[:-1]+[4.100001], times[:-1]+[4.075, 4.1]):
            with self.assertRaises(ValueError): ASSEMBLY.terminal_idle_policy(rows_for(invalid), invalid, 4., LABELS, True)

    def test_relevant_phase_short_idle_and_long_hold_rejected(self):
        times = [0., .6, 1.5, 2.5, 3.5, 3.98, 4.005]
        wrong = dict(LABELS); wrong['83'] = 'ACTUAL FIRE / RELOAD'
        with self.assertRaises(ValueError): ASSEMBLY.terminal_idle_policy(rows_for(times), times, 4., wrong, True)
        rows = rows_for(times)
        for index in range(1, 4): rows[index]['phase'] = '0'
        with self.assertRaises(ValueError): ASSEMBLY.terminal_idle_policy(rows, times, 4., LABELS, True)
        long_hold = [0., .1, 1., 2., 3.4, 4.005]
        with self.assertRaises(ValueError): ASSEMBLY.terminal_idle_policy(rows_for(long_hold), long_hold, 4., LABELS, True)
        wrong_order = [0., 1., .9, 3.99, 4.005]
        with self.assertRaises(ValueError): ASSEMBLY.terminal_idle_policy(rows_for(wrong_order), wrong_order, 4., LABELS, True)

    def test_no_overlap_keeps_strict_behavior(self):
        times = [0., .6, 1.5, 2.5, 3.5, 3.98]
        kept, ledger = ASSEMBLY.terminal_idle_policy(rows_for(times), times, 4., {}, False)
        self.assertEqual(kept, len(times)); self.assertFalse(ledger['applied'])
        with self.assertRaises(ValueError): ASSEMBLY.terminal_idle_policy(rows_for(times), times, 4.6, {}, False)

    def test_all_original_headers_hashes_and_raw_counts_are_bound(self):
        # Minimal JPEG header fixture is sufficient for the assembler's explicit
        # header/dimension validation. No encoder/pixel decoder is invoked here.
        jpeg = b'\xff\xd8\xff\xc0\x00\x07\x08\x00\x02\x00\x02\xff\xd9'
        times = [0., .6, 1.5, 2.5, 3.5, 3.98, 4.005]
        rows = rows_for(times)
        with tempfile.TemporaryDirectory(prefix='c04_terminal_fixture_') as folder:
            folder = Path(folder)
            with (folder/'frames.csv').open('w', newline='', encoding='utf-8') as stream:
                writer = csv.DictWriter(stream, fieldnames=list(rows[0])); writer.writeheader(); writer.writerows(rows)
            (folder/'chapters.csv').write_text('phase,world_seconds,label\n0,2,BEFORE\n83,2.6,'+ASSEMBLY.FINAL_IDLE_LABEL+'\n', encoding='utf-8')
            (folder/'checks.txt').write_text('Complete: 1\nFailures: 0\nFrames: 7\n', encoding='utf-8')
            for row in rows: (folder/row['file']).write_bytes(jpeg)
            with wave.open(str(folder/'gameplay_master.wav'), 'wb') as wav:
                wav.setparams((1, 2, 8000, 0, 'NONE', 'not compressed')); wav.writeframes(b'\0'*64000)
            args = argparse.Namespace(input=folder, chapters=True, phase_labels=None, mode='generic',
                capture_kind='packaged', source_revision='a'*40, source_state='exact-commit', clip_terminal_idle_to_audio=True)
            presented, intervals, _, report = ASSEMBLY.prepare(args)
            self.assertEqual(len(presented), 6); self.assertEqual(report['frames'], 7)
            self.assertEqual(report['presented_frames'], 6); self.assertEqual(intervals[-1][1], 4.)
            expected = hashlib.sha256()
            for row in rows:
                identity = [row['file'], row['audio_seconds'], hashlib.sha256(jpeg).hexdigest()]
                expected.update((json.dumps(identity, ensure_ascii=True, separators=(',', ':'))+'\n').encode())
            self.assertEqual(report['ordered_frame_identity_sha256'], expected.hexdigest())
            self.assertEqual(report['terminal_idle_clip']['omitted_callbacks'][0]['sha256'], hashlib.sha256(jpeg).hexdigest())
            self.assertNotEqual(report['ordered_presented_frame_identity_sha256'], report['ordered_frame_identity_sha256'])
            (folder/rows[-1]['file']).write_bytes(b'not JPEG')
            with self.assertRaises(ValueError): ASSEMBLY.prepare(args)
            (folder/rows[-1]['file']).write_bytes(jpeg)
            (folder/'checks.txt').write_text('Complete: 1\nFailures: 0\nFrames: 6\n', encoding='utf-8')
            with self.assertRaises(ValueError): ASSEMBLY.prepare(args)


if __name__ == '__main__': unittest.main()
