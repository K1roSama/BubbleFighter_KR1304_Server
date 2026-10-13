-- Local ordinary InventorySlot persistence, not the original server's schema.
-- Apply once after CreateCharacters.sql. No starter grants or existing data edits.
CREATE TABLE InventorySlots (
    CharacterId BIGINT UNSIGNED NOT NULL,
    ItemId INT UNSIGNED NOT NULL,
    UsageKind TINYINT UNSIGNED NOT NULL,
    Quantity INT UNSIGNED NOT NULL,
    ExpiresAtUnixSeconds BIGINT UNSIGNED NULL,
    IsEquipped BOOLEAN NOT NULL,
    DetailVersion TINYINT UNSIGNED NOT NULL,
    DetailData MEDIUMBLOB NOT NULL,
    Revision BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (CharacterId, ItemId),
    CONSTRAINT InventorySlotsCharacter FOREIGN KEY (CharacterId) REFERENCES Characters (CharacterId),
    CONSTRAINT InventorySlotsItemId CHECK (ItemId BETWEEN 1 AND 99999999),
    CONSTRAINT InventorySlotsUsage CHECK (UsageKind IN (1, 2, 3)),
    CONSTRAINT InventorySlotsQuantity CHECK (Quantity <= 2147483647),
    CONSTRAINT InventorySlotsExpiry CHECK (ExpiresAtUnixSeconds IS NULL OR
        (UsageKind = 2 AND ExpiresAtUnixSeconds <= 253402300799)),
    CONSTRAINT InventorySlotsEquipped CHECK (IsEquipped IN (0, 1)),
    CONSTRAINT InventorySlotsDetail CHECK (DetailVersion = 1 AND OCTET_LENGTH(DetailData) BETWEEN 29 AND 262144),
    CONSTRAINT InventorySlotsRevision CHECK (Revision > 0)
) ENGINE=InnoDB;
