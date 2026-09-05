"""Remove machine paths from authored FBX/PNG/Blender metadata, preserving art data.

Run in Blender 5.1: --background --disable-autoexec --python-exit-code 1 --python this_script --
    [--apply] [--dependency-path Saved/MetadataDependencies] ArtSource/**/*.fbx ...
Without --apply this only checks. Paths/globs are relative to the checkout.
Compressed .blend support requires zstandard in Blender's Python environment.
No dependencies are downloaded by this script. Backups/reports stay under Saved.
"""
from pathlib import Path
import argparse, array, datetime, gzip, hashlib, io, json, re, struct, sys

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--apply', action='store_true')
parser.add_argument('--dependency-path', type=Path)
parser.add_argument('paths', nargs='+')
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
if args.dependency_path:
    sys.path.insert(0, str((ROOT / args.dependency_path).resolve()))
import bpy
from io_scene_fbx import parse_fbx, encode_bin
import _blendfile_header

PRIVATE = re.compile(rb'(?i)(?:[a-z]:[\\/]+Users[\\/]+|/(?:home|Users)/)')
ABSOLUTE = re.compile(rb'(?i)^(?:[a-z]:[\\/]|/)')
PATH_STRING = re.compile(rb'(?i)(?:[a-z]:[\\/]+Users[\\/]+|/(?:home|Users)/)[^\0\r\n]+')
RUN = ROOT / 'Saved' / 'PublicationAudit' / ('Metadata_' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ'))
RUN.mkdir(parents=True, exist_ok=False)

def allowed(path):
    path = path.resolve()
    relative = path.relative_to(ROOT)
    if relative.parts[0] not in ('ArtSource', 'Evidence'):
        raise ValueError('Only project-authored ArtSource/Evidence files may be changed')
    return path

def sha(data):
    return hashlib.sha256(data).hexdigest()

def commit_bytes(path, original, output, record):
    record.update(path=path.relative_to(ROOT).as_posix(), changed=output != original,
                  original_sha256=sha(original), sanitized_sha256=sha(output))
    if args.apply and output != original:
        backup = RUN / 'OriginalMetadata' / path.relative_to(ROOT)
        backup.parent.mkdir(parents=True, exist_ok=True)
        backup.write_bytes(original)
        assert path.read_bytes() == original, 'Source changed during sanitation'
        path.write_bytes(output)
    return record

def fbx_digest(node):
    digest = hashlib.sha256()
    def add(data):
        digest.update(struct.pack('<Q', len(data))); digest.update(data)
    def visit(elem):
        add(elem.id); add(bytes(elem.props_type)); add(str(len(elem.elems)).encode())
        for prop in elem.props:
            if isinstance(prop, bytes): add(b'bytes'); add(prop)
            elif isinstance(prop, array.array): add(prop.typecode.encode()); add(prop.tobytes())
            elif isinstance(prop, float): add(b'float'); add(struct.pack('<d', prop))
            else: add(type(prop).__name__.encode()); add(str(prop).encode())
        for child in elem.elems: visit(child)
    visit(node)
    return digest.hexdigest()

FBX_METHODS = {ord(k): 'add_' + v for k, v in {
    'B':'bool', 'C':'char', 'Z':'int8', 'Y':'int16', 'I':'int32', 'L':'int64',
    'F':'float32', 'D':'float64', 'R':'bytes', 'S':'string', 'i':'int32_array',
    'l':'int64_array', 'f':'float32_array', 'd':'float64_array',
    'b':'bool_array', 'c':'byte_array'}.items()}

def fbx_encoded(node):
    result = encode_bin.FBXElem(node.id)
    for kind, prop in zip(node.props_type, node.props):
        getattr(result, FBX_METHODS[kind])(prop)
    result.elems = [fbx_encoded(child) for child in node.elems]
    return result

def fbx(path):
    original = path.read_bytes(); tree, version = parse_fbx.parse(str(path)); changes = []
    def visit(node, chain):
        chain = chain + [node.id]
        for i, value in enumerate(node.props):
            if not isinstance(value, bytes): continue
            native_field = (chain == [b'', b'FBXHeaderExtension', b'SceneInfo', b'Properties70', b'P']
                            and i == 4 and node.props[0] in (b'Original|ApplicationNativeFile', b'LastSaved|ApplicationNativeFile'))
            if native_field and ABSOLUTE.search(value):
                source = Path(value.decode()).resolve()
                relative = source.relative_to(ROOT).as_posix().encode()
                node.props[i] = relative
                changes.append(node.props[0].decode())
            elif PRIVATE.search(value):
                raise ValueError('Unrecognized private FBX metadata field; inspect locally before publishing')
        for child in node.elems: visit(child, chain)
    visit(tree, [])
    if not changes: return commit_bytes(path, original, original, {'format':'FBX', 'fields':[]})
    expected = fbx_digest(tree); temporary = RUN / path.name
    saved = encode_bin._write_timedate_hack
    try:
        encode_bin._write_timedate_hack = lambda root: None
        encode_bin.write(str(temporary), fbx_encoded(tree), version)
    finally: encode_bin._write_timedate_hack = saved
    check, check_version = parse_fbx.parse(str(temporary)); output = temporary.read_bytes()
    assert version == check_version and fbx_digest(check) == expected
    assert not PRIVATE.search(output)
    return commit_bytes(path, original, output, {'format':'FBX', 'fields':changes,
                        'all_other_parsed_properties_identical':True, 'semantic_sha256':expected})

def png_chunks(data):
    assert data.startswith(b'\x89PNG\r\n\x1a\n'), 'File extension does not match PNG format'
    cursor = 8; chunks = []
    while cursor < len(data):
        size = struct.unpack_from('>I', data, cursor)[0]; end = cursor + size + 12
        assert end <= len(data)
        chunks.append((data[cursor+4:cursor+8], data[cursor:end])); cursor = end
    return chunks

def png(path):
    original = path.read_bytes(); chunks = png_chunks(original)
    kept = [(kind, chunk) for kind, chunk in chunks if kind not in (b'tEXt', b'zTXt', b'iTXt')]
    output = original[:8] + b''.join(chunk for _, chunk in kept)
    assert png_chunks(output) == kept
    before = b''.join(chunk for kind, chunk in chunks if kind == b'IDAT')
    after = b''.join(chunk for kind, chunk in kept if kind == b'IDAT')
    assert before == after and not PRIVATE.search(output)
    return commit_bytes(path, original, output, {'format':'PNG', 'removed_text_chunks':len(chunks)-len(kept),
                        'all_nontext_chunks_identical':True, 'pixel_IDAT_sha256':sha(after)})

def portable_path_structs(dna):
    """Find exact UI/append-provenance path fields in this file's DNA schema."""
    cursor = 0
    def tag(value):
        nonlocal cursor
        assert dna[cursor:cursor+4] == value; cursor += 4
    def count():
        nonlocal cursor
        value = struct.unpack_from('<I', dna, cursor)[0]; cursor += 4; return value
    def strings():
        nonlocal cursor
        values = []
        for _ in range(count()):
            end = dna.index(b'\0', cursor); values.append(dna[cursor:end].decode()); cursor = end+1
        cursor = (cursor+3) & ~3
        return values
    tag(b'SDNA'); tag(b'NAME'); names = strings(); tag(b'TYPE'); types = strings()
    tag(b'TLEN'); cursor += 2*len(types); cursor = (cursor+3) & ~3; tag(b'STRC')
    indices = set(); weak_libraries = set()
    for i in range(count()):
        kind, size = struct.unpack_from('<HH', dna, cursor); cursor += 4; fields = []
        for _ in range(size):
            ft, fn = struct.unpack_from('<HH', dna, cursor); cursor += 4
            fields.append((types[ft], names[fn]))
        if types[kind] == 'FileSelectParams':
            assert fields[:2] == [('char','title[96]'), ('char','dir[1282]')]
            indices.add(i)
        elif types[kind] == 'LibraryWeakReference':
            # Blender remembers the origin of an appended local datablock.
            # This first fixed char field is a path, not a dependency pointer.
            assert fields[0] == ('char','library_filepath[1024]')
            weak_libraries.add(i)
    return indices, weak_libraries

def blend(path):
    original = path.read_bytes()
    if original.startswith(b'\x28\xb5\x2f\xfd'):
        try: import zstandard
        except ImportError: raise RuntimeError('Install zstandard for Blender Python or pass --dependency-path') from None
        with zstandard.ZstdDecompressor().stream_reader(io.BytesIO(original)) as reader: data = reader.read()
        compress = lambda value: zstandard.ZstdCompressor(level=3).compress(value)
        decompress = lambda value: zstandard.ZstdDecompressor().decompress(value)
    elif original.startswith(b'\x1f\x8b'):
        data = gzip.decompress(original); compress = lambda value: gzip.compress(value, mtime=0)
        decompress = gzip.decompress
    else: data = original; compress = lambda value: value; decompress = lambda value: value
    bpy.ops.wm.open_mainfile(filepath=str(path), load_ui=False, use_scripts=False)
    # Permit only confirmed render-output metadata strings in Scene blocks.
    renders = {}
    for scene in bpy.data.scenes:
        value = scene.render.filepath.encode()
        if PRIVATE.search(value):
            destination = Path(scene.render.filepath).resolve()
            destination.relative_to(ROOT)
            import os
            renders[value] = ('//' + os.path.relpath(destination, path.parent).replace('\\', '/')).encode()
    stream = io.BytesIO(data); header = _blendfile_header.BlendFileHeader(stream)
    assert header.version == 501, 'Review metadata field layout before using a different Blender file version'
    fmt = header.create_block_header_struct(); changed = bytearray(data); edits = []; blocks = []; dna = None
    while stream.tell() < len(data):
        block = _blendfile_header.BlockHeader(stream, fmt); start = stream.tell()
        blocks.append((block, start))
        if block.code == b'DNA1': dna = data[start:start+block.size]
        stream.seek(start+block.size)
        if block.code == b'ENDB': break
    selectors, weak_libraries = portable_path_structs(dna)
    for block, start in blocks:
        for match in PATH_STRING.finditer(data[start:start+block.size]):
            value = match.group()
            if block.code == b'DATA' and block.sdna_index in selectors and match.start() == 96:
                replacement = b'//'; field = 'FileSelectParams.dir'
            elif block.code == b'DATA' and block.sdna_index in weak_libraries and match.start() == 0:
                import os
                destination = Path(value.decode('utf-8')).resolve()
                destination.relative_to(ROOT)
                if not destination.is_file() or destination.suffix.lower() != '.blend':
                    raise ValueError('Appended-library source must be an existing authored Blender file inside the repository')
                relative = os.path.relpath(destination, path.parent).replace('\\', '/')
                assert (path.parent / relative).resolve() == destination
                replacement = ('//' + relative).encode('utf-8')
                field = 'LibraryWeakReference.library_filepath'
            elif block.code == b'SC' and value in renders:
                replacement = renders[value]; field = 'Scene.render.filepath'
            else: raise ValueError('Unrecognized private .blend field; inspect locally before publishing')
            a, b = start+match.start(), start+match.end()
            assert data[b] == 0 and len(replacement) <= b-a
            changed[a:b] = replacement + b'\0' * (b-a-len(replacement))
            edits.append({'field':field, 'offset':a, 'length':b-a})
    assert not PRIVATE.search(changed) and len(data) == len(changed)
    cursor = 0
    for edit in edits:
        a = edit['offset']; assert data[cursor:a] == changed[cursor:a]; cursor = a+edit['length']
    assert data[cursor:] == changed[cursor:]
    output = compress(bytes(changed)) if edits else original
    if edits: assert decompress(output) == bytes(changed)
    result = commit_bytes(path, original, output, {'format':'BLEND', 'fields':edits,
                          'decoded_nonmetadata_bytes_identical':True})
    if args.apply and edits:
        bpy.ops.wm.open_mainfile(filepath=str(path), load_ui=False, use_scripts=False)
        result['reopened_in_blender'] = True
    return result

paths = sorted({allowed(path) for pattern in args.paths for path in ROOT.glob(pattern) if path.is_file()})
if not paths: raise RuntimeError('No matching authored files')
results = []
for path in paths:
    handler = {'.fbx':fbx, '.png':png, '.blend':blend}.get(path.suffix.lower())
    if handler is None: raise ValueError('Unsupported asset extension: ' + path.suffix)
    results.append(handler(path))
report = {'applied':args.apply, 'files':results, 'changed_files':sum(r['changed'] for r in results)}
(RUN/'report.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({'applied':args.apply, 'files_checked':len(results), 'files_with_metadata_changes':report['changed_files'],
                  'report':(RUN/'report.json').relative_to(ROOT).as_posix()}))
