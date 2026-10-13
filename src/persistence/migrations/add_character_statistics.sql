-- Apply once after CreateCharacters.sql. Local schema, not Nexon's database.
-- Existing roles and new roles start at level 1, XP 0, Lucci 0, Cash 0.
-- These are explicit local-account defaults, not recovered original rewards.
-- One ALTER keeps identity and statistics in the same InnoDB row.
ALTER TABLE Characters
    ADD COLUMN Level INT UNSIGNED NOT NULL DEFAULT 1,
    ADD COLUMN Experience INT UNSIGNED NOT NULL DEFAULT 0,
    ADD COLUMN Lucci INT UNSIGNED NOT NULL DEFAULT 0,
    ADD COLUMN Cash INT UNSIGNED NOT NULL DEFAULT 0,
    ADD CONSTRAINT CharactersLevel CHECK (Level BETWEEN 1 AND 2147483647),
    ADD CONSTRAINT CharactersExperience CHECK (Experience <= 2147483647),
    ADD CONSTRAINT CharactersLucci CHECK (Lucci <= 2147483647),
    ADD CONSTRAINT CharactersCash CHECK (Cash <= 2147483647);
