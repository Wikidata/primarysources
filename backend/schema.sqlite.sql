CREATE TABLE snak (
    id        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    property  VARCHAR(32),
    svalue    TEXT,
    dvalue    DECIMAL,
    lat       DOUBLE,
    lng       DOUBLE,
    tvalue    TIMESTAMP,
    precision INT,
    lang      VARCHAR(2),
    vtype     INT
);

CREATE TABLE statement (
    id       INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    subject  VARCHAR(32),
    mainsnak INTEGER REFERENCES snak(id),
    state    INT DEFAULT 0
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

CREATE INDEX idx_stmt_qid ON statement(subject);
