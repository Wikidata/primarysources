CREATE TABLE snak (
    id        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    property  VARCHAR(32),
    svalue    TEXT,
    dvalue    DECIMAL,
    lat       DOUBLE,
    lng       DOUBLE,
    tvalue    DATETIME,
    precision INT,
    lang      VARCHAR(2),
    vtype     INT
);

CREATE TABLE statement (
    id       INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    subject  VARCHAR(32),
    mainsnak INTEGER REFERENCES snak(id),
    state    INT DEFAULT 0,
    dataset  VARCHAR(32),
    upload   INTEGER DEFAULT 0
);

CREATE TABLE qualifier (
    id       INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    stmt     INTEGER REFERENCES statement(id),
    snak     INTEGER REFERENCES snak(id)
);

CREATE TABLE source (
    id       INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    stmt     INTEGER REFERENCES statement(id),
    snak     INTEGER REFERENCES snak(id)
);

CREATE TABLE userlog (
    id       INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    user     VARCHAR(64),
    stmt     INTEGER REFERENCES statement(id),
    state    INT DEFAULT 0,
    changed  TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_stmt_subject_state_dataset ON statement(subject, state, dataset);
CREATE INDEX idx_stmt_state_dataset_subject ON statement(state, dataset, subject);
CREATE INDEX idx_stmst_dataset ON statement(dataset);
CREATE INDEX idx_qualifier_stmt ON qualifier(stmt);
CREATE INDEX idx_source_stmt ON source(stmt);
