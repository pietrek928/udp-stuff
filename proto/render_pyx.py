from enum import Enum
from inspect import getdoc
from .descr import BoolField, EnumField, FloatField, IntField, StringField, StructDescr, StructField, UintField, UnionDescr, UnionField, get_enum_int_mapping
from .render_cc import get_type_name


def render_cdef_union(union: UnionField, parent_name: str):
    yield f"cdef union {parent_name}_{union.name}:"
    for struct in union.structs:
        yield f"    {struct.name} {struct.name}"


def render_ctypedef_struct(struct: StructDescr):
    yield f"ctypedef struct {struct.name}:"
    for field in struct.fields:
        if isinstance(field, UnionField):
            yield f"    int {field.name}_type"
            yield f"    {struct.name}_{field.name} {field.name}"
        else:
            yield f"    {get_type_name(field)} {field.name}"


# def render_struct(struct: StructDescr):
#     yield f"cdef class {struct.name}(BaseModel):"
#     if struct.description:
#         yield f'    """{struct.description}"""'
#     for field in struct.fields:
#         yield f"    {field.name}: {get_type_name(field)}" + (
#             f'  # {field.description}' if field.description else ''
#         )


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


def redner_union2pydantic(union: UnionDescr):
    yield f"cpdef convert_{union.name}2pydantic(const {union.name} &u, {union.name}_type t):"
    if union.allow_empty:
        yield f"    if t == {union.name}_type.{union.name}_empty:"
        yield f"        return pstruct.{union.name}()"
    for i, struct in enumerate(union.union.structs):
        yield f"    {'el' if (i == 0 and union.allow_empty) else ''}if t == {union.name}_type.{struct.name}:"
        yield f"        return pstruct.{union.name}({struct.name}=convert_{struct.name}2pydantic(u.{struct.name}_variant))"
    yield f"    else:"
    yield f"        raise ValueError(f'Invalid union type {t}')"


def render_struct2pydantic(struct: StructDescr):
    yield f"cpdef convert_{struct.name}2pydantic(const {struct.name} &s):"
    yield f"    return pstruct.{struct.name}("
    for f in struct.fields:
        if isinstance(f, EnumField):
            yield f"    {f.name}={f.enum.__name__}((int)s.{f.name}),"
        elif isinstance(f, (BoolField, UintField, IntField, FloatField, StringField)):
            yield f"    {f.name}=s.{f.name},"
        elif isinstance(f, UnionField):
            yield f"    {f.name}=convert_{f.union.name}2pydantic(s.{f.name}, s.{f.name}_type),"
        elif isinstance(f, StructField):
            yield f"    {f.name}=convert_{f.struct.name}2pydantic(s.{f.name}),"
    yield "    )"
    yield "}"

def render_pydantic2union(union: UnionDescr):
    yield f"cpdef convert_pydantic2{union.name}(const pstruct.{union.name} pu, {union.name} &u, {union.name}_type &t):"
    yield f"    t = {union.name}_type(pu.which())"
    for i, struct in enumerate(union.structs):
        yield f"    {'' if i == 0 else 'el'}if t == {union.name}_type.{struct.name}:"
        yield f"        convert_pydantic2{struct.name}(pu.{struct.name}, u.{struct.name}_variant)"


def render_pydantic2struct(struct: StructDescr):
    yield f"cpdef convert_pydantic2{struct.name}(const pstruct.{struct.name} ps, {struct.name} &s):"
    for f in struct.fields:
        if isinstance(f, EnumField):
            yield f"    s.{f.name} = {f.enum.__name__}(ps.{f.name})"
        elif isinstance(f, (BoolField, UintField, IntField, FloatField, StringField)):
            yield f"    s.{f.name} = ps.{f.name}"
        elif isinstance(f, UnionField):
            yield f"    convert_pydantic2{f.union.name}(ps.{f.name}, s.{f.name}, s.{f.name}_type)"
        elif isinstance(f, StructField):
            yield f"    convert_pydantic2{f.struct.name}(ps.{f.name}, s.{f.name})"
