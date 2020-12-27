#!/usr/bin/env python3

#  Copyright (c) 2020, Mike Beaton. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause

"""
Generate OpenCore .c and .h plist config definition files from template plist file.
"""

import io
import os
import sys

import argparse
import base64
import xml.etree.ElementTree as et

# dataclasses print nicely
from dataclasses import dataclass

# Available flags for -f:

# show processed template as we have understood it (use with SHOW_ORIGINAL to recreate original plist)
SHOW_XML = 0x01
# show creation of key objects
SHOW_KEYS = 0x02
# show creation of OC type objects
SHOW_OC_TYPES = 0x04
# show processing steps
SHOW_DEBUG = 0x08
# show additional context used in processing
SHOW_CONTEXT = 0x10
# use with SHOW_PLIST to recreate original plist (i.e. with extra features and hidden elements removed)
SHOW_ORIGINAL = 0x20

# initial flags value
flags = SHOW_XML

# prefix can be changed, to support non-default applications
DEFAULT_PREFIX = 'Oc'

# internal flags passed around during parsing to specify which files we are writing to
OUTPUT_PLIST = 0x1
OUTPUT_C = 0x2
OUTPUT_H = 0x4
OUTPUT_NONE = 0x0
OUTPUT_ALL = OUTPUT_H | OUTPUT_C | OUTPUT_PLIST

# hold .h and .c nodes
@dataclass
class hc:
  h: str
  c: str

  def __init__(self, h=None, c=None):
    self.h = h
    self.c = c

  def merge(self, hc):
    if self.h is None:
      self.h = hc.h
    if self.c is None:
      self.c = hc.c

  def copy(self):
    return hc(self.h, self.c)

# plist key
class plist_key:
  def __init__(self, origin, value, node, this_node, child_node, tab):
    if node is None:
      node = hc(None, None)

    if value is not None:
      if node.h is None:
        node.h = value.upper()
      if node.c is None:
        node.c = value

    if this_node is not None:
      if this_node.c is None:
        this_node.c = node.c
      if this_node.h is None:
        this_node.h = node.h

    self.origin = origin
    self.value = value
    self.node = node
    self.this_node = this_node
    self.child_node = child_node

    plist_key_print('[plist:key', tab=tab, end='')
    if origin != 'key':
      plist_key_attr_print('from', origin)
    plist_key_attr_print('value', value)
    plist_key_attr_print('node', node)
    plist_key_attr_print('this_node', this_node)
    plist_key_attr_print('child_node', child_node)

    plist_key_print(']')

# oc type
class oc_type:
  def __init__(
    self,
    schema_type,
    tab,
    path,
    out_flags,
    default = None,
    size = None,
    of = None,
    ref = None,
    context = None,
    comment = None,
    xref = None,
    suffix = None,
    opt = None
    ):
    if ref is None:
      ref = hc()
    self.name = None
    self.schema_type = schema_type
    self.default = default
    self.path = path
    self.out_flags = out_flags
    self.size = size
    self.of = of
    self.ref = ref
    self.context = context
    self.comment = comment
    self.xref = xref
    self.suffix = suffix
    self.opt = opt
    self.this_node = None

    oc_type_print('[oc:%s' % schema_type, tab=tab, end='')
    oc_type_attr_print('default', default)
    oc_type_attr_print('path', path)
    oc_type_attr_print('size', size)
    of_type = None
    if of is not None:
      if type(of) is list:
        of_type = 'list[%d]' % len(of)
      else:
        of_type = of.schema_type
    oc_type_attr_print('of', of_type)
    oc_type_attr_print('ref', ref)
    oc_type_attr_print('context', context)
    oc_type_attr_print('comment', comment)
    oc_type_attr_print('xref', xref)
    oc_type_attr_print('suffix', suffix)
    oc_type_attr_print('opt', opt)
    oc_type_attr_print('out_flags', out_flags) # displaying last for visibility
    oc_type_print(']')

# error and stop
def error(*args, **kwargs):
  print('ERROR:', *args, file=sys.stderr, **kwargs)
  sys.exit(-1)

# internal error and stop
def internal_error(*args, **kwargs):
  print('INTERNAL_ERROR:', *args, file=sys.stderr, **kwargs)
  sys.exit(-1)

# debug print
def debug(*args, **kwargs):
  if flags & SHOW_DEBUG != 0:
    print('DEBUG:', *args, **kwargs)

# print with tab param and flag control
def info_print(*args, **kwargs):
  if kwargs.pop('info_flags', 0) & flags != 0:
    for _ in range(0, kwargs.pop('tab', 0)):
      print(end='\t')
    print(*args, **kwargs)

# tabbed printing
#

column_pos = {}

def tab_print(*args, **kwargs):
  kwargs['end'] = ''
  kwargs['sep'] = ''
  file = kwargs.get('file', sys.stdout)

  pos = column_pos.get(file, 0)

  s = io.StringIO()

  kwargs['file'] = s

  print(*args, **kwargs)
  count = s.tell()
  column_pos[file] = pos + count
  s.seek(0)

  kwargs['file'] = file
  print(s.read(), **kwargs)

  s.close()

def tab_to(col, file):
  pos = column_pos.get(file, 0)
  count = col - pos
  if count > 0:
    column_pos[file] = col
    for _ in range(count):
      print(' ', file=file, end='')

def tab_nl(file):
  column_pos[file] = 0
  print(file=file)

# additional processing context
def context_print(*args, **kwargs):
  info_print(*args, info_flags=SHOW_CONTEXT, **kwargs)

#
# optionally display objects generated during processing
#

def plist_key_print(*args, **kwargs):
  info_print(*args, info_flags = SHOW_KEYS, **kwargs)

def oc_type_print(*args, **kwargs):
  info_print(*args, info_flags = SHOW_OC_TYPES, **kwargs)

def attr_print(name, value, flags):
  if value is not None:
    info_print(' %s=' % name, info_flags = flags, end='')
    if type(value) is list:
      info_print(value, info_flags = flags, end='')
    elif type(value) is str:
      info_print('"%s"' % value, info_flags = flags, end='')
    else:
      info_print(value, info_flags = flags, end='')

def plist_key_attr_print(name, value):
  attr_print(name, value, SHOW_KEYS)

def oc_type_attr_print(name, value):
  attr_print(name, value, SHOW_OC_TYPES)

# print part of XML output (i.e. processed plist as we have understood it)
def xml_print(*args, **kwargs):
  if kwargs.pop('out_flags', 0) & OUTPUT_PLIST != 0:
    info_print(*args, info_flags = SHOW_XML, **kwargs)

# start of opening XML tag
def start_tag(elem, name, out_flags, tab):
  if elem.tag != name:
    error('expected <%s> but found <%s>' % (name, elem.tag))
  xml_print('<%s' % name, out_flags = out_flags, tab = tab, end = '')

# end of opening XML tag
def end_tag(elem, out_flags):
  xml_print('>', out_flags = out_flags)
  if len(elem.attrib) > 0:
    error('unhandled attributes %s in tag <%s>' % (elem.attrib, elem.tag))

# end and close one-line XML tag
def end_and_close_tag(elem, out_flags, quick_close = True, kill_content = False):
  no_content = kill_content or elem.text is None
  if quick_close and no_content:
    xml_print('/', out_flags = out_flags, end = '')
  else:
    xml_print('>%s</%s' % ('' if no_content else elem.text, elem.tag), out_flags = out_flags, end = '')
  end_tag(elem, out_flags) # abusing 'end' slightly, if we're ending the closing tag

# closing XML tag
def close_tag(elem, out_flags, tab):
  xml_print('</%s>' % elem.tag, out_flags = out_flags, tab = tab)

# display XML attr
def display_attr(name, value, out_flags, displayInOriginal = False):
  if value is not None and (flags & SHOW_ORIGINAL == 0 or displayInOriginal):
    xml_print(' %s="%s"' % (name, value), out_flags = out_flags, end = '')

# consume (and optionally display as we go) XML attr
def consume_attr(elem, name, out_flags, display = True, displayInOriginal = False):
  value = elem.attrib.pop(name, None)
  if display:
    display_attr(name, value, out_flags, displayInOriginal)
  return value

# convert sensible XML or HTML values to Python True or False
def bool_from_str(str_bool):
  if str_bool is None:
    bool_bool = None
  else:
    lower = str_bool.lower()
    if str_bool == '0' or lower == 'false' or lower == 'no':
      bool_bool = False
    elif str_bool == '1' or lower == 'true' or lower == 'yes':
      bool_bool = True
    else:
      error('illegal bool value="', str_bool, '"')
  return bool_bool

# parse attribute controlling which files output goes to
def parse_out_attr(elem, out_flags):
  out_attr = consume_attr(elem, 'out', 0, False)

  if out_attr == None or out_attr == '':
    return (out_flags, out_attr)
  
  attr_flags = dict([(c, True) for c in out_attr.lower()])

  use_flags = out_flags

  if attr_flags.pop('c', None) is None:
    use_flags &= ~OUTPUT_C
    
  if attr_flags.pop('h', None) is None:
    use_flags &= ~OUTPUT_H
    
  if attr_flags.pop('p', None) is None:
    # only stop showing plist output if we are recreating the original plist
    if flags & SHOW_ORIGINAL != 0:
      use_flags &= ~OUTPUT_PLIST

  if len(attr_flags) > 0:
    error('unknown letter flags in attr output="%s", \'c\', \'h\' & \'p\' are allowed' % out_attr)

  return (use_flags, out_attr)

# plist key; option not to use value for is for keys in maps
def parse_key(elem, path, out_flags, tab, use_value = True):
  (use_flags, out_attr) = parse_out_attr(elem, out_flags)

  start_tag(elem, 'key', use_flags, tab)
  h = consume_attr(elem, 'h', use_flags)
  c = consume_attr(elem, 'c', use_flags)
  child_node = fake_node(elem, 'child', out_flags) ## override current node for children only, not this item
  this_node = fake_node(elem, 'this', use_flags) ## override current node to this item only, not children
  display_attr('out', out_attr, use_flags)
  section = consume_attr(elem, 'section', use_flags)
  end_and_close_tag(elem, use_flags)

  key = plist_key('key', elem.text if use_value else None, hc(h, c), this_node, child_node, tab)

  if len(path) == 0:
    emit_section(use_flags, key.value if section is None else section)
  elif section is not None:
    error('section attr in tag <key> only expected at level 1 nesting')

  return (use_flags, key)

# apply key to attach additional info to value
def apply_key(key, oc, tab):
  if oc.name is not None:
    internal_error('name should not get set more than once on oc_type')

  oc_type_print('[oc: ...', tab = tab + 1, end = '')

  oc.name = key.value
  oc_type_print(' name="%s"' % oc.name, end = '')

  if key.this_node is not None:
    oc.this_node = key.this_node
    oc_type_print(' >>>this_node=%s' % oc.this_node, end = '')

  oc_type_print(']')

# data or integer
def parse_data(elem, path, out_flags, tab, is_integer = False):
  data_size = consume_attr(elem, 'size', out_flags)
  data_type = consume_attr(elem, 'type', out_flags)
  default = consume_attr(elem, 'default', out_flags)
  end_and_close_tag(elem, out_flags, quick_close = False)

  if data_type == 'blob':
    if data_size is not None:
      error('attr size should not be used with attr type="blob"')
  elif data_type is not None and data_size is not None:
    pass
  elif is_integer:
    if data_type is None:
      data_type = 'UINT32'
  elif elem.text is not None:
    data_bytes = base64.b64decode(elem.text)
    byte_length = len(data_bytes)

    if data_type is None:
      if byte_length == 2:
        data_type = 'UINT16'
      elif byte_length == 4:
        data_type = 'UINT32'
      elif byte_length == 8:
        data_type = 'UINT64'
      else:
        data_type = 'UINT8'

    # don't automatically convert single byte to array of bytes of size 1
    if data_size is None and data_type == 'UINT8' and byte_length != 1:
      data_size = str(byte_length)
  elif data_type is None:
    if data_size is not None:
      error('attr size should not be used without type on empty <data> tag')
    data_type = 'blob'

  if data_type == 'blob':
    h_ref = 'OC_DATA'
    schema_type = 'blob'
  else:
    h_ref = data_type
    if is_integer:
      schema_type = 'integer'
    else:
      schema_type = 'data'

  return oc_type(schema_type, tab, path, out_flags, default = default, size = data_size, ref = hc(h_ref))

# boolean
def parse_boolean(elem, path, out_flags, tab, value):
  default = consume_attr(elem, 'default', out_flags)
  end_and_close_tag(elem, out_flags)
  return oc_type('boolean', tab, path, out_flags, default = default, ref = hc('BOOLEAN'))

# string
def parse_string(elem, path, out_flags, tab):
  default = consume_attr(elem, 'default', out_flags)
  end_and_close_tag(elem, out_flags, quick_close = False)
  return oc_type('string', tab, path, out_flags, default = default, ref = hc('OC_STRING'))

# pointer (artificial tag to add elements to data definition, cannot be populated from genuine .plist)
def parse_pointer(elem, path, out_flags, tab):
  start_tag(elem, 'pointer', out_flags, tab)
  data_type = consume_attr(elem, 'type', out_flags)
  end_and_close_tag(elem, out_flags)
  return oc_type('pointer', tab, path, out_flags, ref = hc(data_type))

# plist basic types
def parse_basic_type(elem, path, out_flags, tab):
  start_tag(elem, elem.tag, out_flags, tab)

  if elem.tag == 'data':
    retval = parse_data(elem, path, out_flags, tab)
  elif elem.tag == 'integer':
    retval = parse_data(elem, path, out_flags, tab, True)
  elif elem.tag == 'string':
    retval = parse_string(elem, path, out_flags, tab)
  elif elem.tag == 'false':
    retval = parse_boolean(elem, path, out_flags, tab, False)
  elif elem.tag == 'true':
    retval = parse_boolean(elem, path, out_flags, tab, True)
  else:
    error('unexpected tag for basic type, found <%s>' % elem.tag)

  return retval

# check for hide="children", meaning remove all child tags when outputting original plist
def hide_children(elem, out_flags):
  hide = consume_attr(elem, 'hide', out_flags)
  if hide is None:
    hiding = False
  elif hide.lower() == 'children':
    hiding = True
  else:
    error('invalid value for attr hide="%s" in <%s>' % (hide, elem.tag))

  # only hide when recreating original plist
  if flags & SHOW_ORIGINAL == 0:
    hiding = False

  if hiding:
    end_and_close_tag(elem, out_flags, kill_content = True)
    use_flags = out_flags & ~OUTPUT_PLIST
  else:
    end_tag(elem, out_flags)
    use_flags = out_flags
    
  return (hiding, use_flags)

# fake node for overriding certain parts of output
def fake_node(elem, name, out_flags):
  value = consume_attr(elem, name, out_flags)
  h = consume_attr(elem, '%s_h' % name, out_flags)
  c = consume_attr(elem, '%s_c' % name, out_flags)

  if value is None and h is None and c is None:
    return None

  if value is not None:
    if h is None:
      h = value.upper()
    if c is None:
      c = value

  return hc(h, c)

# where node name for children differs from parent
def make_child_path(path, child_node):
  if child_node is None:
    return path

  child_node.merge(path[-1])
  child_path = path.copy()
  del child_path[-1]
  child_path.append(child_node)

  return child_path

# plist array
def parse_array(elem, path, child_node, out_flags, context, tab):
  start_tag(elem, 'array', out_flags, tab)
  suffix = fake_node(elem, 'suffix', out_flags)
  comment = consume_attr(elem, 'comment', out_flags)
  xref = consume_attr(elem, 'xref', out_flags)

  (hiding, use_flags) = hide_children(elem, out_flags)

  child_path = make_child_path(path, child_node)

  index = 0
  value = parse_allowed_type_for_array(elem[index], child_path, None, use_flags, 'array', tab + 1)
  index += 1

  emit_elem(value, tab + 1)

  array_skip(elem, index, child_path, use_flags, 'array', tab)

  if not hiding:
    close_tag(elem, out_flags, tab)

  return oc_type('array', tab, path, out_flags, of = value, ref = hc(xref), context = context, comment = comment, suffix = suffix, xref = xref)

# error if unuasble child attr specified
def useless_child_node(elem, child_node):
  if child_node is not None:
    error('child attribute should only be specified in key preceding <array> or <dict>')

# types allowed inside dict (includes array, and artificial pointer type)
def parse_allowed_type_for_dict(elem, path, child_node, out_flags, context, tab):
  if elem.tag == 'array':
    return parse_array(elem, path, child_node, out_flags, context, tab)
  elif elem.tag == 'pointer':
    useless_child_node(elem, child_node)
    return parse_pointer(elem, path, out_flags, tab)
  else:
    return parse_allowed_type_for_array(elem, path, child_node, out_flags, context, tab)

# types allowed inside array (excludes array itself, and artificial pointer type)
def parse_allowed_type_for_array(elem, path, child_node, out_flags, context, tab):
  if elem.tag == 'dict':
    return parse_dict(elem, path, child_node, out_flags, context, tab)
  else:
    useless_child_node(elem, child_node)
    return parse_basic_type(elem, path, out_flags, tab)

# work out what to do & emit message describing skip if req'd
def init_skip(elem, start, out_flags, tab):
  if flags & SHOW_ORIGINAL == 0:
    # for debug plist, o/p message indicating that these elements are ignored
    use_flags = OUTPUT_NONE
    count = len(elem) - start
    if count > 0:
      xml_print('(suppressing %d item%s)' % (count, '' if count == 1 else 's'), out_flags = out_flags, tab = tab + 1)
  else:
    # for recreated plist, just suppress .h and .c o/p
    use_flags = out_flags & OUTPUT_PLIST
  return use_flags

# skip unused elements
def map_skip(elem, start, path, out_flags, context, tab):
  use_flags = init_skip(elem, start, out_flags, tab)
  index = start
  while index < len(elem):
    (next_flags, _) = parse_key(elem[index], path, use_flags, tab + 1, False)
    index += 1

    parse_allowed_type_for_dict(elem[index], path, None, next_flags, context, tab + 1)
    index += 1

# skip unused elements
def array_skip(elem, start, path, out_flags, context, tab):
  use_flags = init_skip(elem, start, out_flags, tab)
  index = start
  while index < len(elem):
    parse_allowed_type_for_array(elem[index], path, None, use_flags, context, tab + 1)
    index += 1

# parse contents of dict as map
def parse_map(elem, path, comment, hiding, out_flags, use_flags, context, suffix, child_path, xref, tab):
  index = 0

  (next_flags, _) = parse_key(elem[index], child_path, use_flags, tab + 1, False)
  index += 1

  value = parse_allowed_type_for_dict(elem[1], child_path, None, next_flags, 'map', tab + 1)
  index += 1

  emit_elem(value, tab + 1)

  map_skip(elem, index, child_path, use_flags, 'map', tab)

  if not hiding:
    close_tag(elem, out_flags, tab)

  return oc_type('map', tab, path, out_flags, of = value, context = context, comment = comment, suffix = suffix, xref = xref)

# parse contents of dict as struct
def parse_struct(elem, path, comment, hiding, out_flags, use_flags, context, suffix, child_path, opt, xref, tab):
  fields = []

  index = 0
  count = len(elem)

  while index < count:
    (next_flags, key) = parse_key(elem[index], child_path, use_flags, tab + 1)
    index += 1

    new_path = child_path.copy()
    new_path.append(key.node)

    value = parse_allowed_type_for_dict(elem[index], new_path, key.child_node, next_flags, 'struct', tab + 1)
    index += 1

    apply_key(key, value, tab)

    emit_elem(value, tab + 1)

    fields.append(value)

  if not hiding:
    close_tag(elem, out_flags, tab)

  return oc_type('struct', tab, path, out_flags, of = fields, context = context, comment = comment, suffix = suffix, opt = opt, xref = xref)

# error if unsopported attr
def unsupported(attr, attr_name, tag_name):
  if attr is not None:
    error('attr %s not supported on <%s>' % (attr_name, tag_name))

# parse dict (with sub-types struct and map)
def parse_dict(elem, path, child_node, out_flags, context, tab):
  start_tag(elem, 'dict', out_flags, tab)
  dict_type = consume_attr(elem, 'type', out_flags)
  comment = consume_attr(elem, 'comment', out_flags)
  str_opt = consume_attr(elem, 'opt', out_flags)
  xref = consume_attr(elem, 'xref', out_flags)
  suffix = fake_node(elem, 'suffix', out_flags)

  opt = bool_from_str(str_opt)

  (hiding, use_flags) = hide_children(elem, out_flags)

  child_path = make_child_path(path, child_node)

  if dict_type == 'map':
    retval = parse_map(elem, path, comment, hiding, out_flags, use_flags, context, suffix, child_path, xref, tab)
    unsupported(opt, 'opt', 'dict type="map"')
  elif dict_type is not None:
    error('unknown value of attr type="%s" in <dict>' % dict_type)
  else:
    retval = parse_struct(elem, path, comment, hiding, out_flags, use_flags, context, suffix, child_path, opt, xref, tab)

  return retval

# parse root element
def parse_plist(elem, out_flags):
  start_tag(elem, 'plist', out_flags, 0)
  consume_attr(elem, 'version', out_flags, displayInOriginal = True)
  end_tag(elem, out_flags)

  root = parse_dict(elem[0], [], None, out_flags, 'map', 0)

  emit_root_section(out_flags)

  emit_elem(root, 0)

  emit_root_config(root)

  close_tag(elem, out_flags, 0)

# get oc schema type for node
def get_oc_schema_type(elem):
  schema_type = elem.schema_type

  if schema_type == 'string':
    return 'STRING'
  elif schema_type == 'boolean':
    return 'BOOLEAN'
  elif schema_type == 'blob':
    return 'DATA'
  elif schema_type == 'data':
    return 'DATAF'
  elif schema_type == 'integer':
    return 'INTEGER'
  elif schema_type == 'array':
    return 'ARRAY'
  elif schema_type == 'map':
    return 'MAP'
  elif schema_type == 'struct':
    return 'DICT'
  elif schema_type == 'pointer':
    return 'POINTER'
  else:
    internal_error('no mapping to basic OC type for schema_type=\'%s\'' % schema_type)

# get oc structors for node
def get_structors(elem):
  constructor = None
  destructor = '()'

  default = elem.default

  long_destructor = False

  if elem.schema_type == 'string':
    if default is None:
      default = ''
    constructor = 'OC_STRING_CONSTR ("%s", _, __)' % default
    destructor = 'OC_DESTR (OC_STRING)' + ' '
  elif elem.schema_type == 'boolean':
    constructor = 'FALSE'
  elif elem.schema_type == 'data' or elem.schema_type == 'integer':
    if default is None:
      constructor = '0'
    else:
      constructor = default
    if elem.size is not None:
      constructor = '{%s}' % constructor
  elif elem.schema_type == 'map' or elem.schema_type == 'struct' or elem.schema_type == 'array':
    constructor = 'OC_CONSTR%d (%s, _, __)' % (len(elem.path), elem.xref if elem.xref is not None else elem.ref.h)
    destructor = 'OC_DESTR (%s)' % (elem.xref if elem.xref is not None else elem.ref.h)
    long_destructor = True
  elif elem.schema_type == 'blob':
    constructor = 'OC_EDATA_CONSTR (_, __)'
    destructor = 'OC_DESTR (OC_DATA)' + '   '
  elif elem.schema_type == 'pointer':
    constructor = 'NULL'
    destructor = 'OcFreePointer' + ' '
  else:
    internal_error('Unhandled schema type ', elem.schema_type)

  return (long_destructor, constructor, destructor)

# Convert node paths to .c and .h file refs
#

def path_to_ref(path):
  h = '_'.join(filter(None, (p.h for p in path)))
  c = ''.join(filter(None, (p.c for p in path)))
  return hc(h, c)

def set_ref(elem, elem_name, tab):
  use_path = elem.path.copy()
  if elem.this_node is not None:
    del use_path[-1]
    use_path.append(elem.this_node)
  depth = len(elem.path)
  if depth == 0:
    use_path.append(hc('GLOBAL', 'Root'))
  if depth <= 1:
    use_path.append(hc('CONFIG', 'Configuration'))
  if depth > 0 and elem_name is not None:
    use_path.append(elem_name)
  hc_path = path_to_ref(use_path)
  elem.ref = hc(
    '%s_%s' % (upper_prefix, hc_path.h),
    'm%s%s' % (hc_path.c, 'Nodes' if depth == 0 else '')
    )
  oc_type_print('[oc: ... ref=%s]' % elem.ref, tab = tab)

# emit struct field
def emit_field(elem, parent, inside_struct, last):
  out_flags = elem.out_flags

  (long_destructor, constructor, destructor) = get_structors(elem)

  if long_destructor:
    tab_destructor = 112
    tab_end = 149
  else:
    tab_destructor = 105
    tab_end = 130

  if parent.xref is None and out_flags & OUTPUT_H != 0:
    # .h
    tab_to(2, file = h_types)
    tab_print('_(', file = h_types)
    tab_print(elem.xref if elem.xref is not None else elem.ref.h, file = h_types)

    tab_to(36, file = h_types)
    tab_print(', ', elem.path[-1].c, file = h_types)

    tab_to(62, file = h_types)
    tab_print(', ', file = h_types)
    if elem.size is not None:
      tab_print('[%s]' % elem.size, file = h_types)

    tab_to(67, file = h_types)
    tab_print(' , ', file = h_types)
    if constructor is not None:
      tab_print(constructor, file = h_types)

    tab_to(tab_destructor, file = h_types)
    tab_print(' , ', file = h_types)
    if destructor is not None:
      tab_print(destructor, file = h_types)

    tab_print(')', file = h_types)

    if not last:
      tab_to(tab_end, file = h_types)
      tab_print(' \\', file = h_types)

    tab_nl(file = h_types)

  oc_schema_type = get_oc_schema_type(elem)

  if out_flags & OUTPUT_C != 0:
    # .c schema
    tab_to(2, file = c_schema)

    tab_print('OC_SCHEMA_%s%s' % (oc_schema_type, '_IN' if oc_schema_type != 'DICT' else ('_OPT' if elem.opt else '')), file = c_schema)

    tab_to(23, file = c_schema)
    tab_print(' ("%s",' % elem.name, file = c_schema)

    tab_to(51, file = c_schema)

    if oc_schema_type == 'DICT':
      tab_print(' %s' % elem.ref.c, file = c_schema)
    else:
      if inside_struct:
        tab_print(' %s_GLOBAL_CONFIG,' % upper_prefix, file = c_schema)
      else:
        tab_print(' %s,' % (parent.xref if parent.xref is not None else parent.ref.h), file = c_schema)
      tab_to(70, file = c_schema)
      if inside_struct:
        tab_print(' %s' % '.'.join(p.c for p in elem.path), file = c_schema)
      else:
        tab_print(' %s' % elem.name, file = c_schema)

      if oc_schema_type == 'ARRAY' or oc_schema_type == 'MAP':
        tab_print(', &%s' % elem.ref.c, file = c_schema)

    tab_print('),', file = c_schema)

    tab_nl(file = c_schema)

# make these much easier to find in debug output
def add_xref(xref):
  if xref is None:
    return ''
  else:
    return ' xref=%s' % xref

# emit struct
def emit_struct(elem, tab):
  context_print('emit_struct() parent=%s suffix=%s%s' % (elem.context, elem.suffix, add_xref(elem.xref)))

  out_flags = elem.out_flags
  context = elem.context

  suffix = hc() if elem.suffix is None else elem.suffix.copy()

  if context == 'array':
    suffix.merge(hc('ENTRY', 'SchemaEntry'))
  elif context == 'struct':
    suffix.merge(hc(None, 'Schema'))
  elif context == 'map':
    suffix.merge(hc('ARRAY', 'Array'))
  else:
    internal_error('unhandled parent \'%s\' for struct' % context)

  set_ref(elem, suffix, tab)

  emit_comment(elem)

  # show struct context
  def struct_context(file):
    if flags & (SHOW_CONTEXT | SHOW_DEBUG) == (SHOW_CONTEXT | SHOW_DEBUG):
      print('// STRUCT parent=%s%s' % (context, add_xref(elem.xref)), file = file)

  if out_flags & OUTPUT_H != 0:
    # .h
    struct_context(h_types)

    if elem.xref is None:
      print('#define %s_FIELDS(_, __) \\' % elem.ref.h, file = h_types)

  if out_flags & OUTPUT_C != 0:
    # .c schema
    struct_context(c_schema)

    print('STATIC', file = c_schema)
    print('OC_SCHEMA', file = c_schema)
    print('%s[] = {' % elem.ref.c, file = c_schema)

  inside_struct = context == 'struct' ### just use parent?
  last = len(elem.of) - 1
  for (i, field) in enumerate(elem.of):
    emit_field(field, elem, inside_struct, i == last)

  if out_flags & OUTPUT_H != 0:
    # .h
    if elem.xref is None:
      print('  OC_DECLARE (%s)' % elem.ref.h, file = h_types)
      print(file = h_types)

  if out_flags & OUTPUT_C != 0:
    # .c structors
    struct_context(c_structors)

    if elem.xref is None:
      print('OC_STRUCTORS       (%s, ())' % elem.ref.h, file = c_structors)

    # .c schema
    print('};', file = c_schema)
    print(file = c_schema)

# emit array
def emit_array(elem, tab):
  context_print('emit_array() of=%s parent=%s suffix=%s%s' % (elem.of.schema_type, elem.context, elem.suffix, add_xref(elem.xref)))

  out_flags = elem.out_flags
  context = elem.context

  suffix = hc() if elem.suffix is None else elem.suffix.copy()

  if context == 'map':
    suffix.merge(hc('ENTRY', None))
  elif context == 'struct':
    suffix.merge(hc('ARRAY', 'Schema'))
  else:
    internal_error('unhandled parent \'%s\' for array' % context)

  set_ref(elem, suffix, tab)

  array_of = get_oc_schema_type(elem.of)

  emit_comment(elem)

  # show array context
  def array_context(file):
    if flags & (SHOW_CONTEXT | SHOW_DEBUG) == (SHOW_CONTEXT | SHOW_DEBUG):
      print('// ARRAY of=%s parent=%s%s' % (elem.of.schema_type, context, add_xref(elem.xref)), file = file)

  if out_flags & OUTPUT_H != 0:
    # .h
    array_context(h_types)

    if elem.xref is None:
      print('#define %s_FIELDS(_, __) \\' % elem.ref.h, file = h_types)
      print('  OC_ARRAY (%s, _, __)' % (elem.of.xref if elem.of.xref is not None else elem.of.ref.h), file = h_types)
      print('  OC_DECLARE (%s)' % elem.ref.h , file = h_types)
      print(file = h_types)

  if out_flags & OUTPUT_C != 0:
    # .c structors
    array_context(c_structors)

    if elem.xref is None:
      if context == 'map':
        print('OC_STRUCTORS       (%s, ())' % elem.ref.h, file = c_structors)
      else:
        print('OC_ARRAY_STRUCTORS (%s)' % elem.ref.h, file = c_structors)

    # .c schema
    array_context(c_schema)

    if context != 'map':
      print('STATIC', file = c_schema)
      print('OC_SCHEMA', file = c_schema)
      print('%s = OC_SCHEMA_%s (NULL' % (elem.ref.c, array_of), end = '', file = c_schema)
      if array_of == 'DICT':
        print(', %s' % elem.of.ref.c, end = '', file = c_schema)
      print(');', file = c_schema)
      print(file = c_schema)

# emit map
def emit_map_subtype(elem, parent, grandparent, is_map):
  out_flags = elem.out_flags

  map_of = get_oc_schema_type(elem) ### hack if for maps, see 'M' just below

  if out_flags & OUTPUT_C != 0:
    # .c schema
    if flags & (SHOW_CONTEXT | SHOW_DEBUG) == (SHOW_CONTEXT | SHOW_DEBUG):
      print('// MAP OF %s OF %s, SUB-TYPE' % (parent.schema_type, elem.schema_type), file = c_schema)

    print('STATIC' , file = c_schema)
    print('OC_SCHEMA' , file = c_schema)
    print('%sEntry = OC_SCHEMA_%s%s (NULL);' % (grandparent.ref.c, 'M' if is_map else '', map_of), file = c_schema)
    print(file = c_schema)

# emit map
def emit_map(elem, tab):
  # (context does not actually effect map o/p)
  context_print('emit_map() of=%s parent=%s suffix=%s%s' % (elem.of.schema_type, elem.context, elem.suffix, add_xref(elem.xref)))

  out_flags = elem.out_flags
  of = elem.of.schema_type
  
  # map to data blob is predefined type, no need to emit anything
  if of == 'blob':
    elem.ref = hc('OC_ASSOC')
    return

  suffix = hc() if elem.suffix is None else elem.suffix.copy()
  suffix.merge(hc('MAP', 'Schema'))
  set_ref(elem, suffix, tab)

  emit_comment(elem)

  if of == 'array':
    is_map = False
    map_type = 'ARRAY'
  elif of == 'map':
    is_map = True
    map_type = 'MAP'
  else:
    error('unhandled map -> %s', of)

  # currently only support map->array->type or map->map->type
  if elem.xref is None:
    emit_map_subtype(elem.of.of, elem.of, elem, is_map)

  # show map context
  def map_context(file):
    if flags & (SHOW_CONTEXT | SHOW_DEBUG) == (SHOW_CONTEXT | SHOW_DEBUG):
      print('// MAP of=%s%s' % (of, add_xref(elem.xref)), file = file)

  if out_flags & OUTPUT_H != 0:
    # .h
    map_context(h_types)

    if elem.xref is None:
      print('#define %s_FIELDS(_, __) \\' % elem.ref.h, file = h_types)
      print('  OC_MAP (OC_STRING, %s, _, __)' % elem.of.ref.h, file = h_types)
      print('  OC_DECLARE (%s)' % elem.ref.h , file = h_types)
      print(file = h_types)

  if out_flags & OUTPUT_C != 0:
    # .c structors
    map_context(c_structors)

    if elem.xref is None:
      print('OC_MAP_STRUCTORS   (%s)' % elem.ref.h, file = c_structors)

    # .c schema
    map_context(c_schema)

    print('STATIC' , file = c_schema)
    print('OC_SCHEMA' , file = c_schema)
    print('%s = OC_SCHEMA_%s (NULL, &%sEntry);' % (elem.ref.c, map_type, elem.ref.c) , file = c_schema)
    print(file = c_schema)

# emit element
def emit_elem(elem, tab):
  if elem.schema_type == 'array':
    emit_array(elem, tab)
  elif elem.schema_type == 'map':
    emit_map(elem, tab)
  elif elem.schema_type == 'struct':
    emit_struct(elem, tab)

# emit new section name
def emit_section_name(out_flags, h_name, c_name):
  if out_flags & OUTPUT_H != 0:
    # .h
    print('/**', file = h_types)
    print('  %s' % h_name, file = h_types)
    print('**/', file = h_types)
    print(file = h_types)

  if out_flags & OUTPUT_C != 0:
    # .c structors
    # add line space here
    print(file = c_structors)

    # .c schema
    print('//', file = c_schema)
    print('// %s' % c_name, file = c_schema)
    print('//', file = c_schema)
    print(file = c_schema)

# normal section from level 1 key
def emit_section(out_flags, section_name):
  emit_section_name(out_flags, '%s section' % section_name, '%s configuration support' % section_name)

# automatic section name at root
def emit_root_section(out_flags):
  emit_section_name(out_flags, 'Root configuration', 'Root configuration')

# emit comment in into .h file
def emit_comment(elem):
  out_flags = elem.out_flags
  if out_flags & OUTPUT_H != 0:
    if elem.comment is not None:
      print('///', file = h_types)
      print('/// %s.' % elem.comment, file = h_types)
      print('///', file = h_types)

# emit root
def emit_root_config(elem):
  out_flags = elem.out_flags
  if out_flags & OUTPUT_C != 0:
    print('STATIC', file = c_schema)
    print('OC_SCHEMA_INFO', file = c_schema)
    print('mRootConfigurationInfo = {', file = c_schema)
    print(' .Dict = {%s, ARRAY_SIZE (%s)}' % (elem.ref.c, elem.ref.c), file = c_schema)
    print('};', file = c_schema)
    print(file = c_schema)

# if recreating original plist, emit first two lines of template file,
# which otherwise don't show up in processing
def emit_plist_header(out_flags, filename):
  if out_flags & OUTPUT_PLIST != 0:
    if flags & SHOW_ORIGINAL != 0:
      with open(filename) as plist_file:
        for _ in range(2):
          print(next(plist_file), end = '')

# flags
def set_flags(args):
  global flags
  
  if args.f.lower().startswith('0x'):
    flags = int(args.f, 16)
  else:
    flags = int(args.f)
  debug('flags=0x%02X' % flags)

# prefix
def set_prefix(args):
  global camel_prefix
  global upper_prefix

  camel_prefix = args.prefix
  upper_prefix = camel_prefix.upper()
  debug('prefix="%s"/"%s"' % (upper_prefix, camel_prefix))

# read file fragment from fragments directory into string
def read_fragment(dir, filename):
  with open(os.path.join(dir, filename), 'r') as file:
    return file.read()

# add filenames
def add_filenames(args, x_intro):
  program = os.path.basename(sys.argv[0])
  template = os.path.basename(args.infile)
  x_intro = x_intro.replace('[[program]]', program)
  x_intro = x_intro.replace('[[template]]', template)
  return x_intro

# .h file and templates
def set_h_file(args):
  global h_file
  global h_intro
  global h_outro

  if args.h is None:
    h_file = None
  else:
    h_file = args.h
    debug('.h=%s' % h_file)
    h_intro = read_fragment(args.fragments, 'intro.h')
    h_intro = add_filenames(args, h_intro)
    h_outro = read_fragment(args.fragments, 'outro.h')

# .c file and templates
def set_c_file(args):
  global c_file
  global c_intro
  global c_outro
  global h_include

  if args.c is None:
    c_file = None
  else:
    c_file = args.c
    debug('.c=%s' % c_file)
    c_intro = read_fragment(args.fragments, 'intro.c')
    c_intro = add_filenames(args, c_intro)
    c_outro = read_fragment(args.fragments, 'outro.c')

  h_include = args.include

  c_intro = c_intro.replace('[[include]]', h_include)

# string buffers
def init_string_buffers():
  global h_types
  global c_structors
  global c_schema

  h_types = io.StringIO()
  c_structors = io.StringIO()
  c_schema = io.StringIO()

# use argparse library to parse arguments
def parse_args():
  parser = argparse.ArgumentParser(description='Generate OpenCore .c and .h config definition files from template plist file.', add_help=False)
  parser.add_argument('infile', type=str, help='filename of input template .plist file')
  parser.add_argument('--help', '-?', action='help', help='show this help message and exit')
  parser.add_argument('-c', type=argparse.FileType('w'), metavar='c_file', help='filename of output .c file')
  parser.add_argument('-h', type=argparse.FileType('w'), metavar='h_file', help='filename of output .h file')
  default_flags_str = '0x%02x' % flags
  parser.add_argument('-f', type=str, default=default_flags_str, metavar='flags', help='flags to control stdout; options at start of source, default=%s' % default_flags_str)
  parser.add_argument('-o', type=int, default=OUTPUT_ALL, metavar='out_flags', help='debug: initial value of out_flags, default=%s' % OUTPUT_ALL)
  default_h_include = '<Library/OcConfigurationLib.h>'
  parser.add_argument('--fragments', type=str, default=os.path.join(sys.path[0], 'fragments'), metavar='fragdir', help='directory for .c/.h intro/outro templates')
  parser.add_argument('--include', type=str, default=default_h_include, metavar='include', help='.c include, default: %s' % default_h_include)
  parser.add_argument('--prefix', type=str, default=DEFAULT_PREFIX, metavar='prefix', help='prefix for non-default applications, default: "%s"' % DEFAULT_PREFIX)

  args = parser.parse_args()

  # set flags first
  set_flags(args)
  set_prefix(args)
  set_h_file(args)
  set_c_file(args)

  debug('.plist="%s"' % args.infile)

  return (args.o, args.infile)

# customise template with prefix
def customise_template(template):
  return template \
    .replace('[[Prefix]]', camel_prefix) \
    .replace('[[PREFIX]]', upper_prefix)

# output .h and .c files from accumulated buffers
# (.plist file - which depending on options can be re-generation of unmodified plist - is output to stdout as we go)
#

def output_h(out_flags):
  global h_file
  global h_types
  
  if h_file != None:
    h_types.seek(0)

    debug('writing .h')
    if out_flags & OUTPUT_H != 0:
      print(customise_template(h_intro), file=h_file, end='')

    # output buffer contents without flag test here, to allow debug that output was fully supressed everywhere else
    print(h_types.read(), file=h_file, end='')

    if out_flags & OUTPUT_H != 0:
      print(customise_template(h_outro), file=h_file, end='')

    debug('closing .h=%s' % h_file)
    h_file.close()
    h_file = None

  h_types = None

def output_c(out_flags):
  global c_file
  global c_structors
  global c_schema

  if c_file != None:
    c_structors.seek(0)
    c_schema.seek(0)

    debug('writing .c')
    if out_flags & OUTPUT_C != 0:
      print(customise_template(c_intro), file=c_file, end='')

    # output buffer contents without flag test here, to allow debug that output was fully supressed everywhere else
    print(c_structors.read(), file=c_file, end='')
    if out_flags & OUTPUT_C != 0:
      print(file=c_file)
    print(c_schema.read(), file=c_file, end='')

    if out_flags & OUTPUT_C != 0:
      print(customise_template(c_outro), file=c_file, end='')

    debug('closing .c=%s' % c_file)
    c_file.close()
    c_file = None

  c_structors = None
  c_schema = None

# main()
def main():
  (out_flags, infile) = parse_args()

  plist = et.parse(infile).getroot()

  init_string_buffers()

  emit_plist_header(out_flags, infile)
  parse_plist(plist, out_flags)
  
  output_h(out_flags)
  output_c(out_flags)

# go
main()
sys.exit(0)
