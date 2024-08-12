from enum import Enum
from inspect import getdoc
from .descr import (
    ArrayField, BoolField, EnumField, FieldDescr,
    FloatField, IntField, StringField, StructDescr,
    StructField, UintField, get_enum_int_mapping
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


def render_ctypedef(struct: StructDescr):
    yield f"ctypedef struct {struct.name}:"
    for field in struct.fields:
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
