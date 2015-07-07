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

$query = "SELECT DATE_FORMAT(CONVERT_TZ(Date, '+00:00', '+03:00'), '%Y/%m/%d %H:%i') AS DATE,Press,Temp1,Temp2,Temp3,Humid,Lux FROM meteo WHERE Date BETWEEN '$date1' AND '$date2' ORDER BY DATE ASC";
$result = mysql_query($query);

if (!$result) {
	die('Error: ' . mysql_error());
}

$fp = fopen('php://output', 'w');
header('Content-Type: text/csv');	
header('Content-Disposition: inline');

while ($row = mysql_fetch_row($result)) {
	//fputcsv($fp, $row);
	fputs($fp, implode($row, ',')."\n");
}

fclose($fp);
mysql_close($con);
?>
