from enum import Enum
from inspect import getdoc

from .descr import (
    ArrayField, BoolField, EnumField, FieldDescr,
    FloatField, IntField, StringField, StructDescr,
    StructField, UintField, UnionDescr, UnionField, get_enum_int_mapping,
    IdentStart, IdentEnd, Sep
)


def to_camel_case(s: str):
    return ''.join(
        v.capitalize() for v in s.split('_')
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
        if isinstance(field.item, IntField) and field.item.bits <= 8:
            return 'bytes'
        else:
            return f"Tuple[{get_type_name(field.item)}]"
    elif isinstance(field, UnionField):
        return to_camel_case(field.union.name)
    elif isinstance(field, StructField):
        return to_camel_case(field.struct.name)
    else:
        raise ValueError(f"Unsupported field type: {field}")


def render_enum(enum: Enum):
    description = getdoc(enum)
    yield f"class {enum.__name__}(Enum):"
    yield IdentStart
    if description:
        yield f'"""{description}"""'
    for name, value in get_enum_int_mapping(enum):
        description = getdoc(enum.__members__[name])
        yield f"{name} = {value}" + (
            f"  # {description}" if description else ""
        )
    yield IdentEnd
    yield Sep


def render_enum2num_mapping(enum: Enum):
    yield f"{enum.__name__}_2_num = {{"
    yield IdentStart
    for name, value in get_enum_int_mapping(enum):
        yield f"{enum.__name__}.{name}: {value},"
    yield IdentEnd
    yield "}"
    yield Sep


def render_num2enum_mapping(enum: Enum):
    yield f"num_2_{enum.__name__} = {{"
    yield IdentStart
    for name, value in get_enum_int_mapping(enum):
        yield f"{value}: {enum.__name__}.{name},"
    yield IdentEnd
    yield "}"
    yield Sep


def render_union_class(union: UnionDescr):
    # yield f"class {to_camel_case(union.name)}(UnionModel):"
    yield f"class {to_camel_case(union.name)}(BaseModel):"
    yield IdentStart
    if union.description:
        yield f'"""{union.description}"""'
    yield f"_allow_empty = {union.allow_empty}"
    for struct in union.structs:
        yield f"{struct.name}: Optional[{to_camel_case(struct.name)}] = None"
    yield IdentEnd
    yield Sep


def render_struct(struct: StructDescr):
    yield f"class {to_camel_case(struct.name)}(BaseModel):"
    yield IdentStart
    if struct.description:
        yield f'"""{struct.description}"""'
    for field in struct.fields:
        yield f"{field.name}: {get_type_name(field)}" + (
            f"  # {field.description}" if field.description else ""
        )
    yield IdentEnd
    yield Sep


def render_imports():
    yield 'from enum import Enum'
    yield 'from pydantic import BaseModel'
    yield 'from typing import Optional, Tuple'
    yield Sep
