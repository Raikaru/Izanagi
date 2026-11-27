"""Original synthetic ACB/AWB/HCA vectors; no game bytes or per-game key."""
import struct


def utf(columns, rows, name='table', extra_work=0):
    strings = bytearray(b'\0')
    pool = bytearray()
    def string(value):
        offset = len(strings)
        strings.extend(value.encode('ascii') + b'\0')
        return offset
    widths = {'u8': 1, 'u16': 2, 'u32': 4, 'str': 4, 'blob': 8}
    types = {'u8': 0, 'u16': 2, 'u32': 4, 'str': 10, 'blob': 11}
    schema = bytearray()
    for column, kind in columns:
        schema += bytes([0x50 | types[kind]]) + struct.pack('>I', string(column))
    for i in range(extra_work):
        schema += b'\x5b' + struct.pack('>I', string('UnusedWork%d' % i))
    body = bytearray()
    for row in rows:
        for column, kind in columns:
            value = row[column]
            if kind == 'str': body += struct.pack('>I', string(value))
            elif kind == 'blob':
                body += struct.pack('>II', len(pool), len(value)); pool.extend(value)
            else: body += struct.pack({'u8': '>B', 'u16': '>H', 'u32': '>I'}[kind], value)
    table_name = string(name)
    ro = 24 + len(schema); so = ro + len(body); do = so + len(strings)
    header = struct.pack('>HHIIIHHI', 1, ro, so, do, table_name, len(columns) + extra_work,
                         sum(widths[k] for _, k in columns), len(rows))
    packet = header + schema + body + strings + pool
    return b'@UTF' + struct.pack('>I', len(packet)) + packet


def crc16(data):
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8): crc = ((crc << 1) ^ (0x8005 if crc & 0x8000 else 0)) & 0xffff
    return crc


def hca(cipher=0, loop=False, channels=1, rate=44100, blocks=4):
    size = 96
    block_size = 32
    header = bytearray(b'HCA\0' + struct.pack('>HH', 0x200, size))
    header += b'fmt\0' + bytes([channels]) + rate.to_bytes(3, 'big') + struct.pack('>IHH', blocks, 128, 64)
    header += b'comp' + struct.pack('>H', block_size) + bytes([1, 15, 1, 0, 1, 1, 0, 0, 0, 0])
    if loop: header += b'loop' + struct.pack('>IIHH', 1, blocks - 2, 0, 0)
    header += b'ciph' + struct.pack('>H', cipher) + b'pad\0'
    header = header.ljust(size - 2, b'\0')
    header += struct.pack('>H', crc16(header))
    block = b'\xff\xff' + bytes(block_size - 4)
    block += struct.pack('>H', crc16(block))
    return bytes(header) + block * blocks


def awb(entries, offset_width=4, id_width=2, subkey=0):
    count = len(entries); align = 32
    pos = 16 + count * id_width + (count + 1) * offset_width
    offsets = []; payload = bytearray()
    cursor = pos
    for _, data in entries:
        offsets.append(cursor)
        aligned = (cursor + align - 1) // align * align
        payload += bytes(aligned - cursor) + data
        cursor = aligned + len(data)
    offsets.append(cursor)
    return (b'AFS2' + bytes([2, offset_width]) + struct.pack('<HIHH', id_width, count, align, subkey)
            + b''.join(i.to_bytes(id_width, 'little') for i, _ in entries)
            + b''.join(i.to_bytes(offset_width, 'little') for i in offsets) + payload)


def acb(audio=None, streaming=False, extra_work=0, cycle=False, layers=1, sample_limit=None, loop_range=None,
        events=None, sequence_type=0, weights=None, seq_command=None, track_commands=None,
        synth_command=None, nested_synth=False, ratio=100, pallet=65535, omit_type=False,
        sequence_fields=(), sequences=1, codec=2):
    """Named-column port fixture profile, not a claimed complete retail schema."""
    audio = hca() if audio is None else audio
    samples = 4 * 1024 - 128 - 64 if sample_limit is None else sample_limit
    events = [note() + end()] if events is None else events
    track_commands = [None] * len(events) if track_commands is None else track_commands
    if len(track_commands) != len(events):
        raise ValueError('one optional parameter program per track')
    track_rows = []
    command_rows = []
    for i, command in enumerate(track_commands):
        index = 65535 if command is None else len(command_rows)
        if command is not None:
            command_rows.append(dict(Command=command))
        track_rows.append(dict(EventIndex=i,CommandIndex=index))
    synth_rows = [dict(ReferenceItems=struct.pack('>HH', 2 if cycle else 1, 0) * layers,
                       Type=0,CommandIndex=65535 if synth_command is None else 0)]
    if nested_synth:
        synth_rows.insert(0,dict(ReferenceItems=struct.pack('>HH',2,1),Type=0,CommandIndex=65535))
    synth_columns = [('ReferenceItems','blob'),('CommandIndex','u16')]
    sequence_columns = [('NumTracks','u16'),('TrackIndex','blob'),('TrackValues','blob'),
                        ('CommandIndex','u16'),('PlaybackRatio','u16'),('ParameterPallet','u16')]
    if not omit_type:
        synth_columns.append(('Type','u8'))
        sequence_columns.append(('Type','u8'))
    sequence_columns.extend((name,kind) for name,kind,_ in sequence_fields)
    sequence_row = dict(NumTracks=len(events),
                        TrackIndex=b''.join(struct.pack('>H',i) for i in range(len(events))),
                        TrackValues=b'' if weights is None else b''.join(struct.pack('>H',x) for x in weights),
                        CommandIndex=65535 if seq_command is None else 0,PlaybackRatio=ratio,
                        ParameterPallet=pallet,Type=sequence_type)
    sequence_row.update((name,value) for name,_,value in sequence_fields)
    tables = {
        'CueTable': utf([('CueId','u32'),('ReferenceType','u8'),('ReferenceIndex','u16'),('Length','u32')],
                        [dict(CueId=19+i,ReferenceType=3,ReferenceIndex=i,Length=round(samples*1000/44100))
                         for i in range(sequences)]),
        'CueNameTable': utf([('CueName','str'),('CueIndex','u16')],
                            [dict(CueName='synthetic',CueIndex=i) for i in range(sequences)]),
        'WaveformTable': utf([('MemoryAwbId','u16'),('StreamAwbId','u16'),('EncodeType','u8'),('Streaming','u8'),
                              ('NumChannels','u8'),('LoopFlag','u8'),('SamplingRate','u32'),('NumSamples','u32'),('ExtensionData','u16')],
                             [dict(MemoryAwbId=77,StreamAwbId=77,EncodeType=codec,Streaming=int(streaming),
                                   NumChannels=1,LoopFlag=2 if loop_range else 1,SamplingRate=44100,NumSamples=samples,
                                   ExtensionData=0 if loop_range else 65535)]),
        'SynthTable': utf(synth_columns,synth_rows),
        'SequenceTable': utf(sequence_columns,[sequence_row]*sequences),
        'TrackTable': utf([('EventIndex','u16'),('CommandIndex','u16')],track_rows),
        'TrackEventTable': utf([('Command','blob')],[dict(Command=program) for program in events]),
        'AwbFile': b'' if streaming else awb([(77,audio)])
    }
    if seq_command is not None:
        tables['SeqCommandTable'] = utf([('Command','blob')],[dict(Command=seq_command)])
    if command_rows:
        tables['TrackCommandTable'] = utf([('Command','blob')],command_rows)
    if synth_command is not None:
        tables['SynthCommandTable'] = utf([('Command','blob')],[dict(Command=synth_command)])
    if loop_range:
        tables['WaveformExtensionDataTable'] = utf([('LoopStart','u32'),('LoopEnd','u32')],
                                                  [dict(LoopStart=loop_range[0],LoopEnd=loop_range[1])])
    return utf([(key,'blob') for key in tables],[tables],name='Header',extra_work=extra_work)


def note(synth=0):
    return struct.pack('>HBHH',2000,4,2,synth)


def wait(ms):
    return struct.pack('>HBI',2001,4,ms)


def fine(us):
    return struct.pack('>HBh',2005,2,us)


def end():
    return struct.pack('>HB',0,0)


def parameter(opcode, payload):
    return struct.pack('>HB',opcode,len(payload))+payload


def itoc_bank(entries):
    """Memory CPK, including a nonempty UTF DataH table with zero rows."""
    def chunk(magic, packet):
        return magic + struct.pack('<IQ',0xff,len(packet)) + packet
    cols = [('ID','u16'),('FileSize','u32'),('ExtractSize','u32')]
    low = utf(cols,[dict(ID=i,FileSize=len(data),ExtractSize=len(data)) for i,data in entries])
    high = utf(cols,[])
    itoc = utf([('FilesL','u32'),('FilesH','u32'),('DataL','blob'),('DataH','blob')],
               [dict(FilesL=len(entries),FilesH=0,DataL=low,DataH=high)])
    header = utf([('ItocOffset','u32'),('ContentOffset','u32'),('Align','u32')],
                 [dict(ItocOffset=0x400,ContentOffset=0x1000,Align=32)])
    out = bytearray(chunk(b'CPK ',header).ljust(0x400,b'\0'))
    out += chunk(b'ITOC',itoc)
    out = out.ljust(0x1000,b'\0')
    for _,data in sorted(entries):
        out += data
        out += bytes((-len(out))%32)
    return bytes(out)


def adx(frames=65):
    header = (b'\x80\x00\x00\x24\x03\x12\x04\x01' + struct.pack('>IIH',24000,frames,500)
              + b'\x04\x00').ljust(34,b'\0') + b'(c)CRI'
    return header + bytes(18*((frames+31)//32))
