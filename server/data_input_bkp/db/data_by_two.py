"""Sensor Log handler for database"""
from sqlalchemy.orm import Session
from sqlalchemy.sql import text
from db.models import DataByTwo

def add_data_by_two(db: Session, data_by_two: DataByTwo):
    """Add data by two to the database"""
    db.add(data_by_two)
    db.commit()
    db.refresh(data_by_two)
    return data_by_two
