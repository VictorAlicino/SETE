"""Sensor Log handler for database"""
from sqlalchemy.orm import Session
from sqlalchemy.sql import text
from db.models import SensorLog

def add_sensor_log(db: Session, sensor_log: SensorLog):
    """Add sensor log to the database"""
    db.add(sensor_log)
    db.commit()
    db.refresh(sensor_log)
    return sensor_log
