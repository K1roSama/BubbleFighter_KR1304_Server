-- Seventh local migration, after CreateWeaponPresets.sql.
-- Preserve all rows and add the verified ordinary Life (durability) encoding.
ALTER TABLE CashInventorySlots
    DROP CHECK CashInventoryUsage,
    DROP CHECK CashInventoryAmount,
    ADD CONSTRAINT CashInventoryUsage CHECK (UsageKind IN (1, 2, 3)),
    ADD CONSTRAINT CashInventoryAmount CHECK (MaxAmount > 0 AND
        ((UsageKind IN (1, 3) AND Amount BETWEEN 1 AND 65535) OR
         (UsageKind = 2 AND (Amount BETWEEN 1 AND 10000 OR Amount = 4294967295))));
