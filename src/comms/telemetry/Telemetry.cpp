

#include "./Telemetry.h"



Telemetry::Telemetry(PacketIO& _comms) : comms(_comms) {
    this->numRegisters = 0;
};



Telemetry::~Telemetry(){

};


uint8_t Telemetry::id_seed = 0;


void Telemetry::setTelemetryRegisters(uint8_t numRegisters, uint8_t* registers, uint8_t* motors){
    if (numRegisters<=TELEMETRY_MAX_REGISTERS) {
        this->numRegisters = numRegisters;
        for (uint8_t i=0; i<numRegisters; i++) {
            this->registers[i] = registers[i];
            if (motors!=NULL)
                this->registers_motor[i] = motors[i];
            else
                this->registers_motor[i] = 0;
        }
    }
    headerSent = false;
};



void Telemetry::init() {
    this->id = Telemetry::id_seed++;
    headerSent = false;
    if (SimpleFOCRegisters::regs == NULL) {
        SimpleFOCRegisters::regs = new SimpleFOCRegisters();
    }
};



void Telemetry::run() {
    if (numRegisters<1)
        return;
    if (!headerSent) {
        sendHeader();
        headerSent = true;
    }
    if (downsample==0 || downsampleCnt++ < downsample) return;
    downsampleCnt = 0;
    if (min_elapsed_time > 0) {
        unsigned long now = _micros();
        if (now - last_run_time < min_elapsed_time) return;
        last_run_time = now;
    }
    sendTelemetry();
}



void Telemetry::addMotor(FOCMotor* motor) {
    if (numMotors < TELEMETRY_MAX_MOTORS) {
        motors[numMotors] = motor;
        numMotors++;
    }
};



void Telemetry::sendHeader() {
    if (numRegisters > 0) {
        comms << START_PACKET(PacketType::TELEMETRY_HEADER, 2*numRegisters + 1) << id << Separator('=');
        for (uint8_t i = 0; i < numRegisters; i++) {
            comms << registers_motor[i] << Separator(':') << registers[i];
        }
        comms << END_PACKET;
    };
};



void Telemetry::sendTelemetry(){
    if (numRegisters > 0) {
        uint8_t size = 1;
        for (uint8_t i = 0; i < numRegisters; i++) {
            size += SimpleFOCRegisters::regs->sizeOfRegister(registers[i]);
        }
        comms << START_PACKET(PacketType::TELEMETRY, size) << id << Separator('=');
        for (uint8_t i = 0; i < numRegisters; i++) {
            SimpleFOCRegisters::regs->registerToComms(comms, registers[i], motors[registers_motor[i]]);
        };
        comms << END_PACKET;
    }
};
