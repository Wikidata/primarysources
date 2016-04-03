CREATE TABLE snak (
    id        INTEGER NOT NULL AUTO_INCREMENT,
    property  VARCHAR(32),
    svalue    TEXT,
    dvalue    DOUBLE,
    lat       DOUBLE,
    lng       DOUBLE,
    tvalue    DATETIME DEFAULT 0,
    `precision` INT,
    lang      VARCHAR(2),
    vtype     VARCHAR(16),
    PRIMARY KEY (id)
);

CREATE TABLE statement (
    id       INTEGER NOT NULL AUTO_INCREMENT,
    subject  VARCHAR(32),
    mainsnak INTEGER,
    state    INT DEFAULT 0,
    dataset  VARCHAR(32),
    upload   INTEGER DEFAULT 0,
    PRIMARY KEY (id),
    FOREIGN KEY (mainsnak)
        REFERENCES snak(id)
);

CREATE TABLE qualifier (
    id       INTEGER NOT NULL AUTO_INCREMENT,
    stmt     INTEGER REFERENCES statement(id),
    snak     INTEGER REFERENCES snak(id),
    PRIMARY KEY (id),
    FOREIGN KEY (stmt)
        REFERENCES statement(id)
        ON DELETE CASCADE,
    FOREIGN KEY (snak)
        REFERENCES snak(id)
        ON DELETE CASCADE
);

CREATE TABLE source (
    id       INTEGER NOT NULL AUTO_INCREMENT,
    stmt     INTEGER,
    snak     INTEGER,
    PRIMARY KEY (id),
    FOREIGN KEY (stmt)
        REFERENCES statement(id)
        ON DELETE CASCADE,
    FOREIGN KEY (snak)
        REFERENCES snak(id)
        ON DELETE CASCADE
);

CREATE TABLE userlog (
    id       INTEGER NOT NULL AUTO_INCREMENT,
    user     VARCHAR(64),
    stmt     INTEGER,
    state    INT DEFAULT 0,
    changed  TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    FOREIGN KEY (stmt)
        REFERENCES statement(id)
);

CREATE INDEX idx_stmt_subject_state_dataset ON statement(subject, state, dataset);
CREATE INDEX idx_stmt_state_dataset_subject ON statement(state, dataset, subject);
CREATE INDEX idx_stmt_dataset ON statement(dataset);
CREATE INDEX idx_qualifier_stmt ON qualifier(stmt);
CREATE INDEX idx_source_stmt ON source(stmt);
CREATE INDEX idx_userlog_user ON userlog(user);
