<?php
require_once('../config.php');
$dateformat = 'l F j, Y G:i:s';
session_start();
if (!session_is_registered('user')) {
	header("Location: index.php");
	die();
}
$dbh = mysql_connect($dbhost,$dbuser,$dbpass);
mysql_select_db($dbname,$dbh);
?>
<HTML>
<HEAD>
<title>Site admin</title>
</HEAD>
<body bgcolor="#FFFFFF">
<p align="center"><a href="index.php?action=submit">Submit news</a> | <a href="index.php?action=edit">Edit news</a> | <a href="index.php?action=newuser">Create new user</a> | <a href="links.php">Edit links</a> | <a href="index.php?action=downloads">Edit downloads</a> | <a href="index.php?action=chpass">Edit user profile</a> | <a href="index.php?action=config">Misc config</a> | <a href="index.php?action=logout">Logout</a></p>
<p>
<?php
if (isset($_GET['edit'])) {
	$row = mysql_fetch_row(mysql_query("SELECT * FROM linklist WHERE `order` = ".$_GET['edit'].";",$dbh));
	echo "<form action=\"".$_SERVER['PHP_SELF']."?doedit=".$row[3]."\" method=\"post\">\n";
	echo "URL: <input type=\"text\" name=\"url\" value=\"".$row[0]."\"><br />\n";
	echo "target: <input type=\"text\" name=\"target\" value=\"".$row[1]."\"><br />\n";
	echo "name: <input type=\"text\" name=\"name\" value=\"".$row[2]."\"><br />\n";
	echo "<input type=\"submit\" value=\"Update link\">";
	echo "</form>";
} else {
?>

<table width="90%">
<tr bgcolor="#CCCCCC"><td>#</td><td>URL</td><td>target</td><td>name</td><td>edit</td><td>delete</td><td>move up</td><td>move down</td></tr>
<?php
if ($_GET['new'] == 1) {
	$row = mysql_fetch_row(mysql_query("SELECT MAX(`order`)+1 FROM linklist;",$dbh));
	mysql_query("INSERT INTO `linklist`(`order`,`url`,`name`,`target`) VALUES(".$row[0].",'".$_POST['URL']."','".$_POST['name']."','".$_POST['target']."');",$dbh);
} else if ($_GET['doedit']) {
	mysql_query("UPDATE linklist SET `url` = '".$_POST['url']."', `name` = '".$_POST['name']."', `target` = '".$_POST['target']."' WHERE `order` = ".$_GET['doedit'].";",$dbh);
} else if ($_GET['moveup'] || $_GET['movedown']) {
	$query = mysql_query("SELECT name FROM linklist ORDER BY `order` ASC;",$dbh);

	$i = 0;
	if ($row = mysql_fetch_row($query)) {
		do {
			$row2[] = $row;
			$i++;
		}
		while ($row = mysql_fetch_row($query));
		mysql_free_result($query);
	}

	for ($x=0;$x<$i;$x++) {
		$name = $row2[$x][0];
		$order = $x+1;
		if ($_GET['moveup']) {
			if ($order == $_GET['moveup'] - 1) {
				$order2 = $order + 1;
			} else if ($order == $_GET['moveup']) {
				$order2 = $order - 1;
			} else {
				$order2 = $order;
			}
		} else {
			if ($order == $_GET['movedown']) {
				$order2 = $order + 1;
			} else if ($order == $_GET['movedown'] + 1) {
				$order2 = $order - 1;
			} else {
				$order2 = $order;
			}
		}
		mysql_query("UPDATE linklist SET `order` = ".$order2." WHERE `name` = '".$name."';",$dbh);

	}
} else if ($_GET['delete']) {
	mysql_query("DELETE FROM linklist WHERE `order` = ".$_GET['delete'].";",$dbh);
	$query = mysql_query("SELECT name FROM linklist ORDER BY `order` ASC;",$dbh);
	
	$i = 0;
	if ($row = mysql_fetch_row($query)) {
		do {
			$row2[] = $row;
			$i++;
		}
		while ($row = mysql_fetch_row($query));
		mysql_free_result($query);
	}

	for ($x=0;$x<$i;$x++) {
		$name = $row2[$x][0];
		$order = $x+1;
		mysql_query("UPDATE linklist SET `order` = ".$order." WHERE `name` = '".$name."';",$dbh);
	}
}
$max = mysql_fetch_row(mysql_query("SELECT MAX(`order`) FROM linklist;",$dbh));
$query = mysql_query("SELECT * FROM linklist ORDER BY `order` ASC",$dbh);
if ($row = mysql_fetch_row($query)) {
	do {
		echo "<tr><td>".$row[3]."</td><td>".$row[0]."</td><td>".$row[1]."</td><td>".$row[2]."</td><td><a href=\"links.php?edit=".$row[3]."\">edit</a></td><td><a href=\"links.php?delete=".$row[3]."\">delete</a></td>";
		if ($row[3] <> 1) {
			echo "<td><a href=\"links.php?moveup=".$row[3]."\">move up</a></td>";
		} else {
			echo "<td>---</td>";
		}
		if ($row[3] <> $max[0]) {
			echo "<td><a href=\"links.php?movedown=".$row[3]."\">move down</a></td>";
		} else {
			echo "<td>---</td>";
		}
	} while ($row = mysql_fetch_row($query));
} else {
	echo "<tr><td colspan=\"4\">No links</td></tr>";
}
?>
</table>
</p>
<p><form action="<?php echo $_SERVER['PHP_SELF']; ?>?new=1" method="post">
URL <input type="text" name="URL"><br />
target <input type="text" name="target" value="_self"><br />
name <input type="text" name="name"><br />
<input type="submit" value="Add new link">
</form>
<?php
}
?>
</p>
</body>
</HTML>
