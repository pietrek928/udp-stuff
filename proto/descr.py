from enum import Enum
from math import ceil, log2
from typing import Any, Optional, Tuple, Type
from pydantic import BaseModel


class FieldDescr(BaseModel):
    name: str
    description: Optional[str] = None
    default: Optional[Any] = None


class BoolField(FieldDescr):
    pass


class UintField(FieldDescr):
    bits: int


class IntField(FieldDescr):
    bits: int


class FloatField(FieldDescr):
    bits: int


class StringField(FieldDescr):
    size: UintField

    def __init__(self, size_bits: int = 16, **kwargs):
        super().__init__(**kwargs)
        self.size = UintField(name="size", bits=size_bits)


class EnumField(UintField):
    enum: Type[Enum]

    def __init__(self, enum: Type[Enum], bits: Optional[int] = None, **kwargs):
        if bits is None:
            enum_max = max(dict(
                get_enum_int_mapping(enum)
            ).values())
            bits = ceil(log2(enum_max))
        super().__init__(
            enum=enum,
            bits=bits,
            **kwargs
        )


class ArrayField(FieldDescr):
    item: FieldDescr
    size: UintField

    def __init__(self, size_bits: Optional[int] = 16, **kwargs):
        if size_bits is not None:
            kwargs['size'] = UintField(name="size", bits=size_bits)
        super().__init__(**kwargs)


class StructDescr(BaseModel):
    name: str
    description: Optional[str] = None
    fields: Tuple[FieldDescr]


class StructField(FieldDescr):
    struct: StructDescr
    # to be able to append fields to struct in same proto version, use length field
    length: Optional[UintField] = None

    def __init__(self, length_bits: Optional[int] = None, **kwargs):
        if length_bits is not None:
            kwargs['length'] = UintField(name="length", bits=length_bits)
        super().__init__(**kwargs)


class UnionField(FieldDescr):
    structs: Tuple[StructDescr, ...]
    type_field: UintField
    # to be able to append fields to struct in same proto version, use length field
    length: Optional[UintField] = None

    def __init__(self, type_bits: Optional[int] = None, length_bits: Optional[int] = None, **kwargs):
        if kwargs.get('type_field') is None:
            if type_bits is None:
                type_bits = ceil(log2(len(kwargs['structs'])))
            kwargs['type_field'] = UintField(name="type", bits=type_bits)
        if length_bits is not None:
            kwargs['length'] = UintField(name="length", bits=length_bits)
        super().__init__(**kwargs)


def get_enum_int_mapping(enum: Type[Enum]):
    next_num = 0
    for name, value in enum.__members__.items():
        if isinstance(value.value, int):
            if value.value < next_num:
                raise ValueError('Invalid int enum values order')
            yield name, value.value
            next_num = value.value + 1
        else:
            yield name, next_num
            next_num += 1
