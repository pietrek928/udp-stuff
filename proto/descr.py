from enum import Enum
from math import ceil, log2
from typing import Any, Optional, Tuple, Type
from pydantic import BaseModel


IdentStart = object()
IdentEnd = object()
Sep = object()


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
        if 'size' not in kwargs:
            kwargs['size'] = UintField(name="size", bits=size_bits)
        super().__init__(**kwargs)


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
    size: Optional[UintField] = None
    const_size: Optional[int] = None

    def __init__(
        self,
        size_bits: Optional[int] = 16,
        const_size: Optional[int] = None,
        **kwargs
    ):
        if size_bits is not None and const_size is None:
            kwargs['size'] = UintField(name="size", bits=size_bits)
        super().__init__(const_size=const_size, **kwargs)
        assert self.size is not None or self.const_size is not None


class StructDescr(BaseModel):
    name: str
    description: Optional[str] = None
    fields: Tuple[FieldDescr, ...]


class StructField(FieldDescr):
    struct: StructDescr
    # to be able to append fields to struct in same proto version, use length field
    length: Optional[UintField] = None

    def __init__(self, length_bits: Optional[int] = None, **kwargs):
        if length_bits is not None:
            kwargs['length'] = UintField(name="length", bits=length_bits)
        super().__init__(**kwargs)


class UnionDescr(BaseModel):
    name: str
    description: Optional[str] = None
    structs: Tuple[StructDescr, ...]
    allow_empty: bool = False


class UnionField(FieldDescr):
    union: UnionDescr
    type_field: UintField
    # to be able to append fields to structs in same proto version, use length field
    length: Optional[UintField] = None

    def __init__(self, union: UnionDescr, type_bits: Optional[int] = None, length_bits: Optional[int] = None, **kwargs):
        if kwargs.get('type_field') is None:
            if type_bits is None:
                nvariants = len(union.structs) + int(union.allow_empty)
                type_bits = ceil(log2(nvariants))
            kwargs['type_field'] = UintField(name="type", bits=type_bits)
        if length_bits is not None:
            kwargs['length'] = UintField(name="length", bits=length_bits)
        super().__init__(
            union=union,
            **kwargs
        )


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
