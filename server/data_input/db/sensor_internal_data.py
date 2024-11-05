"""Sensor Internal Data handler for database"""
from sqlalchemy.orm import Session
from sqlalchemy.sql import text
from db.models import SensorInternalData

def add_sensor_internal_data(db: Session, sensor_internal_data: SensorInternalData):
    """Add sensor internal data to the database"""
    db.add(sensor_internal_data)
    db.commit()
    db.refresh(sensor_internal_data)
    return sensor_internal_data
