-- this is a migration script to update the database from the original schema to the new schema
-- with datasets and statement timestamps
ALTER TABLE statement ADD COLUMN dataset VARCHAR(32) DEFAULT 'freebase';
ALTER TABLE statement ADD COLUMN upload INTEGER DEFAULT 0;