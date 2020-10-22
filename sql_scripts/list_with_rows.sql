SET @row_number = 0; 
SELECT 
    (@row_number:=@row_number + 1) AS num, 
    hwidhash, 
    ipaddr
FROM
    users
ORDER BY hwidhash, ipaddr
LIMIT 15;