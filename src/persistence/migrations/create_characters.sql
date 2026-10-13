-- Local character identity; not a serialized FighterInfo or original database schema.
-- Apply once after CreateLocalAccounts.sql. No existing tables are altered.
CREATE TABLE Characters (
    CharacterId BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    AccountId BIGINT UNSIGNED NOT NULL,
    NameUtf16 VARBINARY(32) NOT NULL,
    CharacterType INT UNSIGNED NOT NULL,
    ColorIndex INT UNSIGNED NOT NULL,
    CreatedAt TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (CharacterId),
    UNIQUE KEY CharactersAccount (AccountId),
    UNIQUE KEY CharactersName (NameUtf16),
    CONSTRAINT CharactersAccountForeignKey FOREIGN KEY (AccountId) REFERENCES Accounts (AccountId),
    CONSTRAINT CharactersNameLength CHECK (OCTET_LENGTH(NameUtf16) BETWEEN 2 AND 32 AND MOD(OCTET_LENGTH(NameUtf16), 2) = 0),
    CONSTRAINT CharactersSelection CHECK (
        (CharacterType = 10 AND ColorIndex = 7) OR (CharacterType = 19 AND ColorIndex = 4) OR
        (CharacterType = 25 AND ColorIndex = 6) OR (CharacterType = 27 AND ColorIndex = 2) OR
        (CharacterType = 1 AND ColorIndex = 1) OR (CharacterType = 2 AND ColorIndex = 6) OR
        (CharacterType = 11 AND ColorIndex = 2) OR (CharacterType = 12 AND ColorIndex = 6))
) ENGINE=InnoDB;
