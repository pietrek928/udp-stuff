void Connection::disconnect() {
    if (drv) {
        drv->detach(*addr);
        drv = NULL;
    }
}

void Connection::connect(NetDriver *_drv, const NetDriver::NetAddr *_addr) {
    disconnect();
    drv = _drv;
    addr = _addr;
    drv->attach(*addr, this);
}

void Connection::process_control(packet_id_t id, uint8_t *data, size_t len) {
    cout << "Control: len: " << len << " id: " << id << endl;
}

void Connection::process_data(packet_id_t id, uint8_t *data, size_t len) {
    cout << "Data: len: " << len << " id: " << id << endl;
}

//void Connection::parse_cmds() {
    
//}

Connection::~Connection() {
    disconnect();
}

