from sqlalchemy import Column, Integer, Float, String, DateTime
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.dialects.postgresql import UUID
import uuid
from datetime import datetime, tzinfo

Base = declarative_base()

class SensorData(Base):
    __tablename__ = 'sensor_data'

    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)  # Identificador único
    timestamp = Column(DateTime(timezone=True), default=datetime.now())  # Data e hora do registro
    mcu_timestamp = Column(DateTime(timezone=True))  # Data e hora do MCU
    sensor_id = Column(String, nullable=False)  # Identificador do sensor (ex: "sete003/D87C")
    entered = Column(Integer, default=0)  # Contador de entradas
    exited = Column(Integer, default=0)  # Contador de saídas
    gave_up = Column(Integer, default=0)  # Contador de desistências

    def __repr__(self):
        return f"<SensorData(sensor_id='{self.sensor_id}', timestamp='{self.timestamp}', internal_temperature={self.internal_temperature}, free_memory={self.free_memory}, rssi={self.rssi}, uptime={self.uptime}, last_boot_reason={self.last_boot_reason}, entered={self.entered}, exited={self.exited}, gave_up={self.gave_up})>"

class SensorInternalData(Base):
    __tablename__ = 'sensor_internal_data'

    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)  # Identificador único
    timestamp = Column(DateTime(timezone=True), default=datetime.now())  # Data e hora do registro em UTC
    mcu_timestamp = Column(DateTime(timezone=True))  # Data e hora do MCU
    sensor_id = Column(String, nullable=False)  # Identificador do sensor (ex: "sete003/D87C")
    internal_temperature = Column(Float)  # Temperatura interna
    free_memory = Column(Integer)  # Memória livre
    rssi = Column(Float)  # RSSI
    uptime = Column(Integer)  # Tempo de atividade
    last_boot_reason = Column(Integer)  # Última razão de boot

    def __repr__(self):
        return f"<SensorInternalData(sensor_id='{self.sensor_id}', timestamp='{self.timestamp}', internal_temperature={self.internal_temperature}, free_memory={self.free_memory}, rssi={self.rssi}, uptime={self.uptime}, last_boot_reason={self.last_boot_reason})>"

class SensorLog(Base):
    __tablename__ = 'sensor_log'

    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)  # Identificador único
    timestamp = Column(DateTime(timezone=True), default=datetime.now())  # Data e hora do registro em UTC
    mcu_timestamp = Column(DateTime(timezone=True))  # Data e hora do MCU
    sensor_id = Column(String, nullable=False)  # Identificador do sensor (ex: "sete003/D87C")
    log = Column(String, nullable=False)  # Log

    def __repr__(self):
        return f"<SensorLog(sensor_id='{self.sensor_id}', timestamp='{self.timestamp}', log='{self.log}')>"
