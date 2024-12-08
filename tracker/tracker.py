from proto.descr import ArrayField, FloatField, StringField, StructDescr, StructField, UintField, UnionDescr, UnionField
from proto.render_all import render_proto


def peer_id_field(name='id', default=None):
    return ArrayField(
        name=name, item=UintField(name='byte', bits=8),
        const_size=512 // 8, default=default
    )


RegisterPeerStruct = StructDescr(
    name='register_peer',
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
        peer_id_field(),
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
        peer_id_field(default=[]),
        UintField(name='timestamp', bits=64),
        ArrayField(name='stats', item=StructField(name='stat', struct=PeerStat)),
    ]
)

PeerInfo = StructDescr(
    name='peer_info',
    fields=[
        peer_id_field(),
        ArrayField(name='cert', item=UintField(name='byte', bits=8), size_bits=24, default=[]),
        StringField(name='description'),
        ArrayField(name='services', item=StringField(name='service')),
        ArrayField(name='stats', item=StructField(name='stat', struct=PeerStat)),
    ]
)

PeerInfos = StructDescr(
    name='peer_infos',
    fields=[
        ArrayField(
            name='peers',
            item=StructField(name='peer', struct=PeerInfo),
        ),
    ]
)

PeerSearchResult = StructDescr(
    name='peer_search_result',
    fields=[
        ArrayField(
            name='peer_ids',
            item=peer_id_field(),
        ),
    ]
)

TrackerUnion = UnionDescr(
    name='tracker_union',
    structs=[
        RegisterPeerStruct,
        ConnectMessageStruct,
        PutStats,
        PeerInfo,
        PeerInfos,
        PeerSearchResult,
    ]
)

tracker_frame = StructDescr(
    name='tracker_frame',
    fields=[
        UnionField(
            name='tracker_frame_union',
            union=TrackerUnion
        )
    ]
)

render_proto((tracker_frame, ), '/tmp/test_proto')
