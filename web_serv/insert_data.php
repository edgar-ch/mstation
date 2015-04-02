<?php
$con = mysql_connect("db_url", "user", "passw");
if (!$con) {
	die('Connect error' . mysql_error());
}

mysql_select_db("db_name", $con);

$date = date('Y-m-d H:i', strtotime(str_replace('-', '/', $_POST['date'])));
$temp1 = (float) $_POST['temp1'];
$temp2 = (float) $_POST['temp2'];
$humid = (float) $_POST['humid'];

if (!empty($date) && !empty($temp1) && !empty($temp2) && !empty($humid))
{
	$query = "INSERT INTO meteo VALUES('$date','$temp1','$temp2','$humid')";

	if (!mysql_query($query, $con)) {
		die('Error: ' . mysql_error());
	}
}

mysql_close($con);
?>
