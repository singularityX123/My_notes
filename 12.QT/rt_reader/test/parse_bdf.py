#!/usr/bin/env python3
"""Parse EDF/BDF header metadata (BioSemi .bdf is 24-bit BDF)."""
import os
import sys


def parse_bdf(path):
    def field(buf, a, b):
        return buf[a:b].decode('ascii', errors='replace').strip()

    with open(path, 'rb') as f:
        hdr = f.read(256)
        ns = int(field(hdr, 252, 256))  # number of signals
        # EDF/BDF uses a COLUMNAR channel-header layout after the 256-byte main header
        # labels(ns*16) transducer(ns*80) dim(ns*8) pmin/pmax/dmin/dmax(ns*8 each)
        # prefiltering(ns*80) nsamp(ns*8) reserved(ns*32)
        col_labels = f.read(ns * 16)
        col_transd = f.read(ns * 80)
        col_dim    = f.read(ns * 8)
        col_pmin   = f.read(ns * 8)
        col_pmax   = f.read(ns * 8)
        col_dmin   = f.read(ns * 8)
        col_dmax   = f.read(ns * 8)
        col_prefilt = f.read(ns * 80)
        col_nsamp  = f.read(ns * 8)
        f.read(ns * 32)  # reserved

    version = field(hdr, 0, 8)
    patient = field(hdr, 8, 88)
    recording = field(hdr, 88, 168)
    startdate = field(hdr, 168, 176)   # dd.MM.yy
    starttime = field(hdr, 176, 184)   # hh.mm.ss
    hdr_bytes = int(field(hdr, 184, 192))
    reserved = field(hdr, 192, 236).strip()
    n_records = int(field(hdr, 236, 244))
    rec_dur = float(field(hdr, 244, 252))  # seconds per data record

    def col(buf, i, width):
        return field(buf, i * width, (i + 1) * width)

    labels = [col(col_labels, i, 16) for i in range(ns)]
    dims = [col(col_dim, i, 8) for i in range(ns)]
    pmin = [float(col(col_pmin, i, 8)) for i in range(ns)]
    pmax = [float(col(col_pmax, i, 8)) for i in range(ns)]
    dmin = [int(float(col(col_dmin, i, 8))) for i in range(ns)]
    dmax = [int(float(col(col_dmax, i, 8))) for i in range(ns)]
    sps = [int(col(col_nsamp, i, 8)) for i in range(ns)]  # samples per record

    n_total = sum(sps)
    fs = sps[0] / rec_dur if rec_dur else 0.0
    data_bytes = os.path.getsize(path) - hdr_bytes
    total_samples = data_bytes / (3 * n_total) if n_total else 0
    duration = total_samples / fs if fs else 0.0
    return dict(
        path=os.path.basename(path), version=version, patient=patient[:60],
        recording=recording, startdate=startdate, starttime=starttime,
        hdr_bytes=hdr_bytes, reserved=reserved, n_records=n_records,
        rec_dur=rec_dur, ns=ns, labels=labels, dims=dims, fs=fs,
        pmin=pmin, pmax=pmax, dmin=dmin, dmax=dmax, sps=sps,
        data_bytes=data_bytes,
        total_samples=int(total_samples), duration_min=round(duration / 60.0, 1),
    )


if __name__ == '__main__':
    for p in sys.argv[1:]:
        try:
            d = parse_bdf(p)
            print('====', d['path'])
            print('  version=', d['version'], '| patient=', repr(d['patient']))
            print('  start=', d['startdate'], d['starttime'],
                  '| hdr_bytes=', d['hdr_bytes'])
            print('  reserved=', repr(d['reserved']),
                  '| n_data_records=', d['n_records'],
                  '| rec_dur(s)=', d['rec_dur'])
            print('  n_channels=', d['ns'], '| fs=', d['fs'], 'Hz')
            print('  labels=', d['labels'])
            print('  dims=', d['dims'])
            print('  total_samples=', d['total_samples'],
                  '| duration=', d['duration_min'], 'min')
            print('  pmin=', d['pmin'], 'pmax=', d['pmax'])
            print('  data_bytes=', d['data_bytes'])
        except Exception as e:
            print(p, 'ERR', repr(e))
