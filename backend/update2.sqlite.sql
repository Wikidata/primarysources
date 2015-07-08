CREATE INDEX IF NOT EXISTS idx_stmt_subject_state_dataset ON statement(subject, state, dataset);
DROP INDEX IF EXISTS idx_stmt_qid;
CREATE INDEX IF NOT EXISTS idx_stmt_state_dataset_subject ON statement(state, dataset, subject);
DROP INDEX IF EXISTS idx_stmt_state;
