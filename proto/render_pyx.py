from enum import Enum
from inspect import getdoc
from .descr import (
    ArrayField, BoolField, EnumField, FieldDescr, FloatField, IntField, StringField,
    StructDescr, StructField, UintField, UnionDescr, UnionField, get_enum_int_mapping,
    IdentStart, IdentEnd, Sep
)


def get_type_name(field: FieldDescr):
    if isinstance(field, EnumField):
        return field.enum.__name__
    elif isinstance(field, UintField):
        if field.bits <= 8:
            return "unsigned char"
        elif field.bits <= 16:
            return "unsigned short"
        elif field.bits <= 32:
            return "unsigned int"
        elif field.bits <= 64:
            return "unsigned long long"
        else:
            raise ValueError(f"Unsupported uint field size: {field.bits}")
    elif isinstance(field, IntField):
        if field.bits <= 8:
            return "char"
        elif field.bits <= 16:
            return "short"
        elif field.bits <= 32:
            return "int"
        elif field.bits <= 64:
            return "long long"
        else:
            raise ValueError(f"Unsupported int field size: {field.bits}")
    elif isinstance(field, FloatField):
        if field.bits <= 32:
            return "float"
        elif field.bits <= 64:
            return "double"
        else:
            raise ValueError(f"Unsupported float field size: {field.bits}")
    elif isinstance(field, BoolField):
        return "bool"
    elif isinstance(field, StringField):
        return "string"
    elif isinstance(field, EnumField):
        return field.enum.__name__
    elif isinstance(field, ArrayField):
        return f"vector[{get_type_name(field.item)}]"
    elif isinstance(field, StructField):
        return field.struct.name
    elif isinstance(field, UnionField):
        return field.union.name
    else:
        raise ValueError(f"Unsupported field type: {field}")



def render_cdef_union(union: UnionField):
    yield f"cdef union {union.name}:"
    yield IdentStart
    for struct in union.structs:
        yield f"{struct.name} {struct.name}_variant"
    yield IdentEnd
    yield Sep


def render_ctypedef_struct(struct: StructDescr):
    yield f"ctypedef struct {struct.name}:"
    yield IdentStart
    for field in struct.fields:
        if isinstance(field, UnionField):
            yield f"{field.union.name}_enum_class {field.name}_type"
        yield f"{get_type_name(field)} {field.name}"
    yield IdentEnd
    yield Sep


def render_enum(enum: Enum):
    description = getdoc(enum)
    yield f'ctypedef enum {enum.__name__}:'
    yield IdentStart
    if description:
        yield f'"""{description}"""'
    for name, _ in get_enum_int_mapping(enum):
        description = getdoc(enum.__members__[name])
        yield f"{name}," + (
            f'  # {description}' if description else ''
        )
    yield IdentEnd
    yield Sep


def render_union(union: UnionDescr):
    yield f'cdef union {union.name}:'
    yield IdentStart
    if union.description:
        yield f'"""{union.description}"""'
    for struct in union.structs:
        yield f"{struct.name} {struct.name}_variant"
    yield IdentEnd
    yield Sep


def render_union_enum(union: UnionDescr):
    yield f'ctypedef enum {union.name}_enum_class:'
    yield IdentStart
    if union.allow_empty:
        yield f'{union.name}_empty'
    for struct in union.structs:
        yield f"{struct.name}"
    yield IdentEnd
    yield Sep


def redner_union2pydantic(union: UnionDescr):
    yield f"cpdef convert_{union.name}2pydantic(const {union.name} &u, {union.name}_enum_class t):"
    yield IdentStart
    if union.allow_empty:
        yield f"if t == {union.name}_enum_class.{union.name}_empty:"
        yield IdentStart
        yield f"return pstruct.{union.name}()"
        yield IdentEnd
    for i, struct in enumerate(union.union.structs):
        yield f"{'el' if (i == 0 and union.allow_empty) else ''}if t == {union.name}_enum_class.{struct.name}:"
        yield IdentStart
        yield f"return pstruct.{union.name}({struct.name}=convert_{struct.name}2pydantic(u.{struct.name}_variant))"
        yield IdentEnd
    yield f"else:"
    yield IdentStart
    yield f"raise ValueError(f'Invalid union type {{t}}')"
    yield IdentEnd
    yield Sep


def render_struct2pydantic(struct: StructDescr):
    yield f"cpdef convert_{struct.name}2pydantic(const {struct.name} &s):"
    yield IdentStart
    yield f"return pstruct.{struct.name}("
    yield IdentStart
    for f in struct.fields:
        if isinstance(f, EnumField):
            yield f"{f.name}={f.enum.__name__}((int)s.{f.name}),"
        elif isinstance(f, (BoolField, UintField, IntField, FloatField, StringField)):
            yield f"{f.name}=s.{f.name},"
        elif isinstance(f, UnionField):
            yield f"{f.name}=convert_{f.union.name}2pydantic(s.{f.name}, s.{f.name}_type),"
        elif isinstance(f, StructField):
            yield f"{f.name}=convert_{f.struct.name}2pydantic(s.{f.name}),"
    yield IdentEnd
    yield "    )"
    yield IdentEnd
    yield Sep


def render_pydantic2union(union: UnionDescr):
    yield f"cpdef convert_pydantic2{union.name}(const pstruct.{union.name} pu, {union.name} &u, {union.name}_type &t):"
    yield IdentStart
    yield f"t = {union.name}_type(pu.which())"
    for i, struct in enumerate(union.structs):
        yield f"{'' if i == 0 else 'el'}if t == {union.name}_type.{struct.name}:"
        yield IdentStart
        yield f"convert_pydantic2{struct.name}(pu.{struct.name}, u.{struct.name}_variant)"
        yield IdentEnd
    yield IdentEnd
    yield Sep


def render_pydantic2struct(struct: StructDescr):
    yield f"cpdef convert_pydantic2{struct.name}(const pstruct.{struct.name} ps, {struct.name} &s):"
    yield IdentStart
    for f in struct.fields:
        if isinstance(f, EnumField):
            yield f"s.{f.name} = {f.enum.__name__}(ps.{f.name})"
        elif isinstance(f, (BoolField, UintField, IntField, FloatField, StringField)):
            yield f"s.{f.name} = ps.{f.name}"
        elif isinstance(f, UnionField):
            yield f"convert_pydantic2{f.union.name}(ps.{f.name}, s.{f.name}, s.{f.name}_type)"
        elif isinstance(f, StructField):
            yield f"convert_pydantic2{f.struct.name}(ps.{f.name}, s.{f.name})"
    yield IdentEnd
    yield Sep


def render_imports():
    yield 'from libcpp.string cimport string'
    yield 'from libcpp.vector cimport vector'
    yield Sep
