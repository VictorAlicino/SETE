"""Database ORM"""
import sys
import logging
from decouple import config as env
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from db.models import Base
from configurations.singleton_metaclass import SingletonMeta
from contextlib import contextmanager

# Logger
log = logging.getLogger(__name__)

class DB(metaclass=SingletonMeta):
    """Database ORM"""
    def __init__(self):
        self.engine = create_engine(env('DB_URL'))
        self.session_local = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        self.attemp_connect()
        log.info("Connected to database")

    @contextmanager
    def get_db(self):
        """Get the database session"""
        db = self.session_local()
        try:
            yield db
        finally:
            db.close()

    def attemp_connect(self):
        """Attempt to connect to the database"""
        try:
            log.debug("Testing database connection...")
            self.engine.connect()
        except Exception as e:
            print(e)
            log.critical("Failed to connect to database")
            sys.exit(1)

    def create_all(self):
        """Create all tables"""
        log.debug("Creating tables if they don't already exists")
        Base.metadata.create_all(bind=self.engine)
