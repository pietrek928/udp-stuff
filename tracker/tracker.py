from proto.descr import ArrayField, FloatField, StringField, StructDescr, StructField, UintField, UnionDescr, UnionField
from proto.render_all import render_proto


RegisterStruct = StructDescr(
    name='register',
    fields=[
        # TODO: services here ?
        UintField(name='ipv4', bits=32, default=0),
        StringField(name='description'),
        ArrayField(name='services', item=StringField(name='service')),
    ]
)

ConnectMessageStruct = StructDescr(
    name='connect_request',
    fields=[
        ArrayField(name='id', item=UintField(name='byte', bits=8), const_size=512 // 8),
        ArrayField(name='cert', item=UintField(name='byte', bits=8), size_bits=24, default=[]),  # ???
        UintField(name='ipv4', bits=32, default=0),
        UintField(name='ipv4_port', bits=16),
        UintField(name='timestamp', bits=64, default=0),
        StringField(name='description', default=''),
    ]
)

PeerStat = StructDescr(
    name='peer_stat',
    fields=[
        StringField(name='name'),  # TODO: mapped int ???
        FloatField(name='value', bits=32),
    ]
)

PutStats = StructDescr(
    name='put_stats',
    fields=[
        ArrayField(name='id', item=UintField(name='byte', bits=8), const_size=512 // 8, default=[]),
        UintField(name='timestamp', bits=64),
        ArrayField(name='stats', item=StructField(name='stat', struct=PeerStat)),
    ]
)

PeerInfo = StructDescr(
    name='peer_info',
    fields=[
        ArrayField(name='id', item=UintField(name='byte', bits=8), const_size=512 // 8),
        ArrayField(name='cert', item=UintField(name='byte', bits=8), size_bits=24, default=[]),
        StringField(name='description'),
        ArrayField(name='services', item=StringField(name='service')),
        ArrayField(name='stats', item=StructField(name='stat', struct=PeerStat)),
    ]
)

TrackerUnion = UnionDescr(
    name='tracker_union',
    structs=[
        RegisterStruct,
        ConnectMessageStruct,
        PutStats,
        PeerInfo,
    ]
)

tracker_frame = StructDescr(
    name='tracker',
    fields=[
        UnionField(
            name='tracker',
            union=TrackerUnion
        )
    ]
)

render_proto((tracker_frame, ), '/tmp/test_proto')
