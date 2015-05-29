<?php
$con = mysql_connect("db_url", "user", "passw");
if (!$con) {
	die('Connect error' . mysql_error());
}

mysql_select_db("db_name", $con);

$date = date('Y-m-d H:i:s', strtotime($_POST['date']));
$press = (int) $_POST['press'];
$temp1 = (float) $_POST['temp1'];
$temp2 = (float) $_POST['temp2'];
$temp2 = (float) $_POST['temp3'];
$humid = (int) $_POST['humid'];
$lux = (float) $_POST['lux'];

if (!empty($date) && !empty($temp1) && !empty($temp2) && !empty($humid))
{
	$query = "INSERT INTO
	meteo_test (`Date`, `Press`, `Temp1`, `Temp2`, `Temp3`, `Humid`, `Lux`)
	VALUES('$date','$press','$temp1','$temp2','$temp2','$humid','$lux')";

	if (!mysql_query($query, $con)) {
		die('Error: ' . mysql_error());
	}
}

mysql_close($con);
?>
