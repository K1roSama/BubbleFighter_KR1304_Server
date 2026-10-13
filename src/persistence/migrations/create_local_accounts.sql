-- Local-account design; no claim that these columns are original game fields.
-- Apply once, using the separate schema administrator on the selected database.
CREATE TABLE Accounts (
    AccountId BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    Username VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    PasswordSalt BINARY(16) NOT NULL,
    PasswordHash BINARY(32) NOT NULL,
    PasswordIterations INT UNSIGNED NOT NULL,
    IsDisabled BOOLEAN NOT NULL DEFAULT FALSE,
    CreatedAt TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (AccountId),
    UNIQUE KEY AccountsUsername (Username),
    CONSTRAINT AccountsIterations CHECK (PasswordIterations BETWEEN 600000 AND 1000000),
    CONSTRAINT AccountsDisabled CHECK (IsDisabled IN (0, 1))
) ENGINE=InnoDB;

CREATE TABLE LoginTickets (
    TicketHash BINARY(32) NOT NULL,
    AccountId BIGINT UNSIGNED NOT NULL,
    CreatedAt TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    ExpiresAt DATETIME(6) NOT NULL,
    ConsumedAt DATETIME(6) NULL,
    PRIMARY KEY (TicketHash),
    KEY LoginTicketsAccount (AccountId),
    KEY LoginTicketsExpiry (ExpiresAt),
    CONSTRAINT LoginTicketsAccountForeignKey FOREIGN KEY (AccountId) REFERENCES Accounts (AccountId) ON DELETE CASCADE
) ENGINE=InnoDB;
