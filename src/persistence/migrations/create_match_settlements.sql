-- Eighth migration. Local idempotent match receipts, not an original schema.
-- Apply once as administrator after the character/statistics migrations.
-- No IF NOT EXISTS: reject an existing incompatible table without overwriting it.
CREATE TABLE MatchSettlements (
    MatchId BINARY(16) NOT NULL,
    RequestData VARBINARY(4096) NOT NULL,
    ResultData VARBINARY(4096) NOT NULL,
    CommittedAt TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (MatchId)
) ENGINE=InnoDB;
