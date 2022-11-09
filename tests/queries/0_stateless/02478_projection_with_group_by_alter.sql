CREATE TABLE testing
(
    a String,
    b String,
    c Int32,
    d Int32,
    PROJECTION proj_1
    (
        SELECT c ORDER BY d
    )
)
ENGINE = MergeTree() PRIMARY KEY (a) SETTINGS min_bytes_for_wide_part = 0;

INSERT INTO testing SELECT randomString(5), randomString(5), rand32(), rand32() FROM numbers(10);

OPTIMIZE TABLE testing FINAL;

-- update all colums used by proj_1
ALTER TABLE testing UPDATE c = c+1, d = d+2 WHERE True SETTINGS mutations_sync=2;

SELECT * FROM system.mutations WHERE database = currentDatabase() AND table = 'testing' AND not is_done;

-- update only one column
ALTER TABLE testing UPDATE d = d-1 WHERE True SETTINGS mutations_sync=2;

SELECT * FROM system.mutations WHERE database = currentDatabase() AND table = 'testing' AND not is_done;

-- update only another one column
ALTER TABLE testing UPDATE c = c-1 WHERE True SETTINGS mutations_sync=2;

SELECT * FROM system.mutations WHERE database = currentDatabase() AND table = 'testing' AND not is_done;

