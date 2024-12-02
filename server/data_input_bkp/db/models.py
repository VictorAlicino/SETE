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
    sensor_id = Column(String, nullable=False)  # Identificador do sensor (ex: "sete003/D87C")
    entered = Column(Integer, default=0)  # Contador de entradas
    exited = Column(Integer, default=0)  # Contador de saídas
    gave_up = Column(Integer, default=0)  # Contador de desistências

    def __repr__(self):
        return f"<SensorData(sensor_id='{self.sensor_id}', timestamp='{self.timestamp}', internal_temperature={self.internal_temperature}, free_memory={self.free_memory}, rssi={self.rssi}, uptime={self.uptime}, last_boot_reason={self.last_boot_reason}, entered={self.entered}, exited={self.exited}, gave_up={self.gave_up})>"

class DataByTwo(Base):
    __tablename__ = 'data_by_two'

    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    timestamp = Column(DateTime(timezone=True), default=datetime.now)
    sensor_id = Column(String, nullable=False)
    traversed = Column(Integer, default=0)

    def __repr__(self):
        return f"<data_by_two(id={self.id}, sensor_id='{self.sensor_id}', timestamp='{self.timestamp}', traversed={self.traversed})>"
