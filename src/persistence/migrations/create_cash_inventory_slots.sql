-- Local pending cash purchases. Apply once after the four prior migrations.
-- No balance credits, starter items or edits to existing accounts/characters.
CREATE TABLE CashInventorySequence (
    SequenceId TINYINT UNSIGNED NOT NULL PRIMARY KEY,
    NextSlotId INT UNSIGNED NOT NULL,
    CONSTRAINT CashInventorySequenceId CHECK (SequenceId = 1),
    CONSTRAINT CashInventorySequenceRange CHECK (NextSlotId BETWEEN 1 AND 100000000)
) ENGINE=InnoDB;
INSERT INTO CashInventorySequence (SequenceId, NextSlotId) VALUES (1, 1);

CREATE TABLE CashInventorySlots (
    SlotId INT UNSIGNED NOT NULL PRIMARY KEY,
    CharacterId BIGINT UNSIGNED NOT NULL,
    StockId INT UNSIGNED NOT NULL,
    ItemId INT UNSIGNED NOT NULL,
    UsageKind TINYINT UNSIGNED NOT NULL,
    Amount INT UNSIGNED NOT NULL,
    MaxAmount INT UNSIGNED NOT NULL,
    CashPrice INT UNSIGNED NOT NULL,
    PurchasedAtUnixSeconds BIGINT UNSIGNED NOT NULL,
    KEY CashInventoryOwner (CharacterId, SlotId),
    CONSTRAINT CashInventoryCharacter FOREIGN KEY (CharacterId) REFERENCES Characters (CharacterId),
    CONSTRAINT CashInventorySlotRange CHECK (SlotId BETWEEN 1 AND 99999999),
    CONSTRAINT CashInventoryItemRange CHECK (ItemId BETWEEN 1 AND 99999999 AND StockId > 0),
    CONSTRAINT CashInventoryUsage CHECK (UsageKind IN (1, 2)),
    CONSTRAINT CashInventoryAmount CHECK (MaxAmount > 0 AND
        ((UsageKind = 1 AND Amount BETWEEN 1 AND 65535) OR
         (UsageKind = 2 AND (Amount BETWEEN 1 AND 10000 OR Amount = 4294967295)))),
    CONSTRAINT CashInventoryPrice CHECK (CashPrice <= 2147483647),
    CONSTRAINT CashInventoryPurchaseTime CHECK (PurchasedAtUnixSeconds <= 253402300799)
) ENGINE=InnoDB;
