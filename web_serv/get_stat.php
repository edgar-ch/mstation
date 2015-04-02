<?php
$con = mysql_connect("db_url", "user", "passw");
if (!$con) {
	die('Connect error' . mysql_error());
}

mysql_select_db("db_name", $con);

$date1 = $_GET['date1'];
$date2 = $_GET['date2'];

if (empty($date1) || empty($date2)) {
	die('Empty fields.');
}

$min_temp_query = "SELECT DATE_FORMAT(`Date`, '%Y-%m-%d %H:%i') AS `date_min`, `Temp1` AS `temp_min` FROM `meteo`
WHERE ( `Date` BETWEEN '$date1' AND '$date2' )
AND `Temp1` = ( SELECT MIN(`Temp1`) FROM `meteo` WHERE `Date` BETWEEN '$date1' AND '$date2' )";

$max_temp_query = "SELECT DATE_FORMAT(`Date`, '%Y-%m-%d %H:%i') AS `date_max`, `Temp1` AS `temp_max` FROM `meteo`
WHERE ( `Date` BETWEEN '$date1' AND '$date2' )
AND `Temp1` = ( SELECT MAX(`Temp1`) FROM `meteo` WHERE `Date` BETWEEN '$date1' AND '$date2' )";

$min_temp = mysql_query($min_temp_query);
if (!$min_temp) {
	die('Error select min temperature: ' . mysql_error());
}

$max_temp = mysql_query($max_temp_query);
if (!$min_temp) {
	die('Error select max temperature: ' . mysql_error());
}

$final = mysql_fetch_assoc($min_temp) + mysql_fetch_assoc($max_temp);
echo json_encode($final);
?>
