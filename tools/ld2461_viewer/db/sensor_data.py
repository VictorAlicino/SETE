"""Sensor Data handler for database"""
from sqlalchemy.orm import Session
from sqlalchemy.sql import text
from db.models import SensorData

def add_sensor_data(db: Session, sensor_data: SensorData):
    """Add sensor data to the database"""
    db.add(sensor_data)
    db.commit()
    db.refresh(sensor_data)
    return sensor_data
