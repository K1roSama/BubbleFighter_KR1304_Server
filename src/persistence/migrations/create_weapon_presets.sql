-- Local primary-weapon preset state. Apply once after inventory migrations.
-- Five initial pages are the user's local policy. No rows, items, unlock cards or balances
-- are granted by this migration. Other wire weapon positions remain unknown.
CREATE TABLE WeaponPresetState (
    CharacterId BIGINT UNSIGNED NOT NULL,
    ActiveIndex TINYINT UNSIGNED NOT NULL,
    PageCount TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (CharacterId),
    CONSTRAINT FK_WeaponPresetState_Character FOREIGN KEY (CharacterId)
        REFERENCES Characters (CharacterId),
    CONSTRAINT CK_WeaponPresetState_Pages CHECK (PageCount BETWEEN 1 AND 5 AND ActiveIndex < PageCount)
) ENGINE = InnoDB;

CREATE TABLE WeaponPresetSlots (
    CharacterId BIGINT UNSIGNED NOT NULL,
    PresetIndex TINYINT UNSIGNED NOT NULL,
    PrimaryItemId INT UNSIGNED NOT NULL,
    PRIMARY KEY (CharacterId, PresetIndex),
    CONSTRAINT FK_WeaponPresetSlots_State FOREIGN KEY (CharacterId)
        REFERENCES WeaponPresetState (CharacterId),
    CONSTRAINT CK_WeaponPresetSlots_Index CHECK (PresetIndex < 5),
    CONSTRAINT CK_WeaponPresetSlots_Item CHECK (PrimaryItemId = 0 OR PrimaryItemId BETWEEN 200001 AND 299999)
) ENGINE = InnoDB;
