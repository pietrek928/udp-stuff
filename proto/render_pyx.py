from enum import Enum
from inspect import getdoc
from .descr import StructDescr, UnionDescr, UnionField, get_enum_int_mapping
from .render_cc import get_type_name


def render_struct(struct: StructDescr):
    yield f"cdef class {struct.name}(BaseModel):"
    if struct.description:
        yield f'    """{struct.description}"""'
    for field in struct.fields:
        yield f"    {field.name}: {get_type_name(field)}" + (
            f'  # {field.description}' if field.description else ''
        )


def render_enum(enum: Enum):
    description = getdoc(enum)
    yield f'ctypedef enum {enum.__name__}:'
    if description:
        yield f'    """{description}"""'
    for name, _ in get_enum_int_mapping(enum):
        description = getdoc(enum.__members__[name])
        yield f"    {name}," + (
            f'  # {description}' if description else ''
        )


def render_union(union: UnionDescr):
    yield f'cdef union {union.name}:'
    if union.description:
        yield f'    """{union.description}"""'
    for struct in union.structs:
        yield f"    {struct.name} {struct.name}_variant"


def render_union_enum(union: UnionField):
    yield f'ctypedef enum {union.name}_type:'
    if union.union.allow_empty:
        yield f'    {union.name}_empty,'
        n = 1
    else:
        n = 0
    for struct in union.union.structs:
        yield f"    {struct.name},"
        n += 1
