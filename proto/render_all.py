from collections import defaultdict
from enum import Enum
from os import makedirs, path
from typing import Dict, Iterable, Union

from .descr import ArrayField, EnumField, IdentEnd, IdentStart, StructDescr, StructField, UnionDescr, UnionField
from . import render_cc, render_py, render_pyx

class FileOutput:
    def __init__(self, f):
        self.f = f
        self.ident = 0
        self.__exit__ = f.__exit__

    def __enter__(self, *a, **k):
        self.f.__enter__(*a, **k)
        return self

    def __exit__(self, *a, **k):
        return self.f.__exit__(*a, **k)

    def put(self, items: Iterable):
        for item in items:
            if item is IdentStart:
                self.ident += 1
            elif item is IdentEnd:
                self.ident -= 1
            else:
                idents = '    ' * self.ident
                self.f.write(f'{idents}{item}\n')

DescrObjType = Union[StructDescr, UnionDescr, Enum]

def get_name(o: DescrObjType):
    if isinstance(o, Enum):
        return o.__name__
    return o.name


def get_children(o: DescrObjType):
    if isinstance(o, StructDescr):
        for f in o.fields:
            if isinstance(f, ArrayField):
                f = f.item

            if isinstance(f, StructField):
                yield f.struct
            elif isinstance(f, UnionField):
                yield f.union
            elif isinstance(f, EnumField):
                yield f.enum
    elif isinstance(o, UnionDescr):
        yield from o.structs


def validate_duplicates(start_objs: Iterable[Union[StructDescr, UnionDescr]]):
    objs_by_name = {}
    objs_stack = list(start_objs)
    while objs_stack:
        o = objs_stack.pop()
        name = get_name(o)
        if name in objs_by_name:
            if o != objs_by_name[name]:
                raise ValueError(f'Item `{name}` different by same name {o} != {objs_by_name[name]}')
            continue

        objs_by_name[name] = o
        objs_stack.extend(get_children(o))


def count_entries(start_objs: Iterable[Union[StructDescr, UnionDescr]]):
    visited = set()
    entries_count = defaultdict(int)
    objs_stack = list(start_objs)
    while objs_stack:
        o = objs_stack.pop()
        name = get_name(o)
        entries_count[name] += 1
        if name in visited:
            continue

        visited.add(name)
        objs_stack.extend(get_children(o))

    return dict(entries_count)


def order_descrs(
    start_objs: Iterable[Union[StructDescr, UnionDescr]],
    entries_count: Dict[str, int]
):
    order = []
    entries_count = dict(entries_count)
    objs_stack = list(start_objs)

    while objs_stack:
        o = objs_stack.pop()
        name = get_name(o)
        entries_count[name] -= 1
        if entries_count[name]:
            continue

        order.append(o)
        objs_stack.extend(
            sorted(get_children(o), key=get_name)
        )

    return tuple(reversed(order))


def render_proto(
    head_descrs: Iterable[Union[StructDescr, UnionDescr]],
    out_prefix: str
):
    out_dir = path.dirname(out_prefix)
    if out_dir:
        makedirs(out_dir, exist_ok=True)
    head_descrs = tuple(head_descrs)
    validate_duplicates(head_descrs)
    entries_cnt = count_entries(head_descrs)
    ordered_objs = order_descrs(head_descrs, entries_cnt)

    with (
        FileOutput(open(f'{out_prefix}.h', 'w')) as h_out,
        FileOutput(open(f'{out_prefix}.py', 'w')) as py_out,
        FileOutput(open(f'{out_prefix}.pyx', 'w')) as pyx_out,
    ):
        h_out.put(render_cc.render_header_begin())
        py_out.put(render_py.render_imports())

        for o in ordered_objs:
            if isinstance(o, StructDescr):
                h_out.put(render_cc.render_struct(o))
                h_out.put(render_cc.render_parse_struct(o))
                py_out.put(render_py.render_struct(o))
                pyx_out.put(render_pyx.render_ctypedef_struct(o))
            elif isinstance(o, UnionField):
                h_out.put(render_cc.render_union_enum(o))
                h_out.put(render_cc.render_union(o))
                # h_out.put(render_cc.render_parse_union(o))
                py_out.put(render_cc.render_union_enum(o))
                py_out.put(render_py.render_union_class(o))
                pyx_out.put(render_pyx.render_union_enum(o))
                pyx_out.put(render_pyx.render_union(o))
            elif isinstance(o, Enum):
                h_out.put(render_cc.render_enum(o))
                py_out.put(render_py.render_enum(o))
                pyx_out.put(render_pyx.render_enum(o))
