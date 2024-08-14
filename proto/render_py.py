from enum import Enum
from inspect import getdoc

from .descr import (
    ArrayField, BoolField, EnumField, FieldDescr,
    FloatField, IntField, StringField, StructDescr,
    StructField, UintField, UnionField, get_enum_int_mapping
)


def get_type_name(field: FieldDescr):
    if isinstance(field, (UintField, IntField)):
        return "int"
    elif isinstance(field, FloatField):
        return "float"
    elif isinstance(field, BoolField):
        return "bool"
    elif isinstance(field, StringField):
        return "str"
    elif isinstance(field, EnumField):
        return field.enum.__name__
    elif isinstance(field, ArrayField):
        return f"vector[{get_type_name(field.item)}]"
    elif isinstance(field, StructField):
        return f"{field.struct.name}"
    else:
        raise ValueError(f"Unsupported field type: {field}")


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


def render_enum(enum: Enum):
    description = getdoc(enum)
    yield f"class {enum.__name__}(Enum):"
    if description:
        yield f'    """{description}"""'
    for name, value in get_enum_int_mapping(enum):
        description = getdoc(enum.__members__[name])
        yield f"    {name} = {value}" + (
            f"  # {description}" if description else ""
        )


def render_enum2num_mapping(enum: Enum):
    yield f"{enum.__name__}_2_num = {{"
    for name, value in get_enum_int_mapping(enum):
        yield f"    {enum.__name__}.{name}: {value},"
    yield "}"


def render_num2enum_mapping(enum: Enum):
    yield f"num_2_{enum.__name__} = {{"
    for name, value in get_enum_int_mapping(enum):
        yield f"    {value}: {enum.__name__}.{name},"
    yield "}"


def render_union_class(union: UnionField, parent_name: str):
    yield f"class {parent_name}_{union.name}(UnionModel):"
    if union.description:
        yield f'    """{union.description}"""'
    for struct in union.structs:
        yield f"    {struct.name}: Optional[{struct.name}] = None"


def render_struct(struct: StructDescr):
    yield f"class {struct.name}(BaseModel):"
    if struct.description:
        yield f'    """{struct.description}"""'
    for field in struct.fields:
        yield f"    {field.name}: {get_type_name(field)}" + (
            f"  # {field.description}" if field.description else ""
        )
