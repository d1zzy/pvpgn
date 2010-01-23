<?php
require_once('../config.php');
$dateformat = 'l F j, Y G:i:s';
session_start();
if ($_GET['action'] == 'logout' || $_GET['action'] == 'dochpass') {
	$uid = $_SESSION['uid'];
	$_SESSION = array();
	if (isset($_COOKIE[session_name()])) {
		setcookie(session_name(),'',time()-42000,'/');
	}
	session_destroy();
}
?>
<html>
<head>
<title>Site admin</title>
</head>
<body bgcolor="#FFFFFF">
<?php
if (!session_is_registered('user') && $_GET['action'] != 'login' && $_GET['action'] != 'dochpass') {
?>
<p align="center"><form action="<?php echo $_SERVER['PHP_SELF']; ?>?action=login" method="post">
Username: <input type="text" name="username" maxlength="32"><br />
Password: <input type="password" name="password"><br />
<input type="submit" value="Log in">
</form></p>
<?php
} else {
	if ($_GET['action'] == 'login' || $_GET['action'] == 'donewuser' || $_GET['action'] == 'dosubmit' || $_GET['action'] == 'downloads' || $_GET['action'] == 'dodownloads' || $_GET['action'] == 'edit' || $_GET['action'] == 'dodelete' || $_GET['action'] == 'doedit' || $_GET['action'] == 'edititem' || $_GET['action'] == 'chpass' || $_GET['action'] == 'dochpass' || $_GET['action'] == 'config' || $_GET['action'] == 'doconfig') {
		$dbh = mysql_connect($dbhost,$dbuser,$dbpass);
		mysql_select_db($dbname,$dbh);
	}
	if ($_GET['action'] == 'login') {
		$row = mysql_fetch_row(mysql_query("SELECT * FROM users WHERE username = '".$_POST['username']."';",$dbh));
		if (!$row) {
			echo "<p align=\"center\">User \"".$_POST['username']."\" does not exist</p>\n";
		} else if ($row[2] != md5($_POST['password'])) {
			echo "<p align=\"center\">Password incorrect</p>\n";
		} else {
			$_SESSION['uid'] = $row[0];
			$_SESSION['user'] = $row[1];
			$_SESSION['email'] = $row[3];
			echo "<p align=\"center\">Login successful</p>\n";
			echo "<p align=\"center\"><a href=\"".$_SERVER['PHP_SELF']."\">Click here to continue</a></p>\n";
		}
	} else {
		if ($_GET['action'] != 'logout' && $_GET['action'] != 'dochpass') {
			?>
			<p align="center"><a href="<?php echo $_SERVER['PHP_SELF']; ?>?action=submit">Submit news</a> | <a href="<?php echo $_SERVER['PHP_SELF']; ?>?action=edit">Edit news</a> | <a href="<?php echo $_SERVER['PHP_SELF']; ?>?action=newuser">Create new user</a> | <a href="links.php">Edit links</a> | <a href="<?php echo $_SERVER['PHP_SELF']; ?>?action=downloads">Edit downloads</a> | <a href="<?php echo $_SERVER['PHP_SELF']; ?>?action=chpass">Edit user profile</a> | <a href="<?php echo $_SERVER['PHP_SELF']; ?>?action=config">Misc config</a> | <a href="<?php echo $_SERVER['PHP_SELF']; ?>?action=logout">Logout</a></p>
			<?php
		}
		if ($_GET['action'] == 'newuser') {
		?>
		<p><form action="<?php echo $_SERVER['PHP_SELF']; ?>?action=donewuser" method="post">
		Username: <input type="text" name="username" maxlength="32"><br />
		Password: <input type="password" name="pass1"><br />
		Confirm password: <input type="password" name="pass2"><br />
		Email address: <input type="text" name="email" maxlength="128"><br />
		<input type="submit" value="Create new user">
		</form></p>
		<?php
		} else if ($_GET['action'] == 'donewuser') {
			if ($_POST['pass1'] <> $_POST['pass2']) {
				echo "<p align=\"center\">Password and repeated password do not match</p>\n";
			} else {
				if (@mysql_query("INSERT INTO `users`(`username`,`passhash`,`email`) VALUES('".$_POST['username']."','".md5($_POST['pass1'])."','".$_POST['email']."');",$dbh)) {
					echo "<p align=\"center\">User \"".$_POST['username']."\" created successfully</p>\n";
				} else {
					echo "<p align=\"center\">Error: Could not create new user</p>\n";
					echo "<p align=\"center\">MySQL said: ".mysql_error()."</p>\n";
				}
			}
		} else if ($_GET['action'] == 'submit') {
			echo "<p><form action=\"".$_SERVER['PHP_SELF']."?action=dosubmit\" method=\"post\">\n";
			echo "<b>Username:</b> ".$_SESSION['user']."<br />\n";
			echo "<b>Email:</b> ".$_SESSION['email']."<br />\n";
			echo "<b>Date:</b> ".date($dateformat)."<br />\n";
		?>
		<b>Subject:</b> <input type="text" size="50" name="subject" maxlength="128"><br />
		<b>News text:</b><br />
		<textarea name="text" cols="50" rows="10"></textarea><br />
		<input type="submit" value="Submit news">
		</form></p>
		<?php
		} else if ($_GET['action'] == 'dosubmit') {
			if (@mysql_query("INSERT INTO `news` VALUES(".time().",".$_SESSION['uid'].",'".$_POST['subject']."','".$_POST['text']."');",$dbh)) {
				echo "<p align=\"center\">News added successfully</p>\n";
			} else {
				echo "<p align=\"center\">Error: Could not insert news</p>\n";
				echo "<p align=\"center\">MySQL said: ".mysql_error()."</p>\n";		
			}
		} else if ($_GET['action'] == 'edit') {
			echo "<div align=\"center\"><table width=\"90%\">";
			echo "<tr bgcolor=\"#CCCCCC\"><td>Timestamp</td><td>Subject</td><td>Edit</td><td>Delete</td></tr>\n";
			$query = mysql_query("SELECT timestamp,subject FROM news ORDER BY `timestamp` DESC",$dbh);
			if ($row = mysql_fetch_row($query)) {
				do {
					echo "<tr><td>".$row[0]."</td><td>".$row[1]."</td><td><a href=\"".$_SERVER['PHP_SELF']."?action=edititem&x=".$row[0]."\">edit</a></td><td><a href=\"".$_SERVER['PHP_SELF']."?action=dodelete&x=".$row[0]."\">delete</a></td>\n";
				} while ($row = mysql_fetch_row($query));
			} else {
				echo "<tr><td colspan=\"4\">No news is good news!</td></tr>\n";
			}
			echo "</table></div>\n";
		} else if ($_GET['action'] == 'dodelete') {
			mysql_query("DELETE FROM news WHERE `timestamp` = ".$_GET['x'].";",$dbh);
			echo "<p align=\"center\">Newsitem deleted</p>\n";
		} else if ($_GET['action'] == 'edititem') {
			$row = mysql_fetch_row($temp = mysql_query("SELECT subject,text FROM news WHERE `timestamp` = ".$_GET['x'].";",$dbh));
			echo "<p><form action=\"".$_SERVER['PHP_SELF']."?action=doedit&x=".$_GET['x']."\" method=\"post\">\n";
			echo "Date: ".date($dateformat,$_GET['x'])."<br />\n";
			echo "Subject: <input type=\"text\" name=\"subject\" value=\"".$row[0]."\"><br />\n";
			echo "<textarea name=\"text\" cols=\"50\" rows=\"10\">".$row[1]."</textarea><br />\n";
			echo "<input type=\"submit\" value=\"Update news\">\n";
			echo "</form></p>\n";
			mysql_free_result($temp);
			unset($temp);
			unset($row);
		} else if ($_GET['action'] == 'doedit') {
			if (@mysql_query("UPDATE news SET `subject` = '".$_POST['subject']."', `text` = '".$_POST['text']."' WHERE `timestamp` = ".$_GET['x'].";",$dbh)) {
				echo "<p align=\"center\">News updated successfully</p>\n";
			} else {
				echo "<p align=\"center\">Error: Could not update news</p>\n";
				echo "<p align=\"center\">MySQL said: ".mysql_error()."</p>\n";		
			}
		} else if ($_GET['action'] == 'chpass') {
			$row = mysql_fetch_row(mysql_query("SELECT username,email FROM users WHERE uid = ".$_SESSION['uid'].";",$dbh));
			?>
			<p><form action="<?php echo $_SERVER['PHP_SELF']; ?>?action=dochpass" method="post">
			Username: <input type="text" name="username" value="<?php echo $row[0]; ?>" maxlength="32"><br />
			Password: <input type="password" name="pass1"><br />
			Confirm password: <input type="password" name="pass2"><br />
			Email address: <input type="text" name="email" value="<?php echo $row[1]; ?>" maxlength="128"><br />
			<input type="submit" value="Apply changes">
			</form></p>
			<?php
		} else if ($_GET['action'] == 'dochpass') {
			if ($_POST['pass1'] == $_POST['pass2']) {
				if (@mysql_query("UPDATE users SET `username` = '".$_POST['username']."', `passhash` = '".md5($_POST['pass1'])."', `email` = '".$_POST['email']."' WHERE `uid` = ".$uid.";",$dbh)) {
					echo "<p align=\"center\">Profile updated successfully</p>\n";
					echo "<p align=\"center\">You must <a href=\"".$_SERVER['PHP_SELF']."\">log in</a> again before you can perform further actions.</p>\n";
				} else {
					echo "<p align=\"center\">Error: Could not update profile</p>\n";
					echo "<p align=\"center\">MySQL said: ".mysql_error()."</p>\n";
				}
			} else {
				echo "<p align=\"center\">Error: Password and repeated password do not match</p>\n";
			}
		} else if ($_GET['action'] == 'downloads') {
			$row = mysql_fetch_row($temp = mysql_query("SELECT value FROM config WHERE `key` = 'latest_stable'",$dbh));
			$latest_stable = $row[0];
			mysql_free_result($temp);
			$row = mysql_fetch_row($temp = mysql_query("SELECT value FROM config WHERE `key` = 'latest_development'",$dbh));
			$latest_development = $row[0];
			mysql_free_result($temp);
			$row = mysql_fetch_row($temp = mysql_query("SELECT value FROM config WHERE `key` = 'latest_d2pack109'",$dbh));
			$latest_d2pack109 = $row[0];
			mysql_free_result($temp);
			unset($temp);
			unset($row);
			echo "<form action=\"".$_SERVER['PHP_SELF']."?action=dodownloads\" method=\"post\">\n";
			echo "Latest development release: <input type=\"text\" name=\"development\" value=\"".$latest_development."\"> (enter 0 to disable)<br />\n";
			echo "<a href=\"downloads.php?type=development\">Edit files for development</a><br />\n<br />\n";
			echo "Latest stable release: <input type=\"text\" name=\"stable\" value=\"".$latest_stable."\"><br />\n";
			echo "<a href=\"downloads.php?type=stable\">Edit files for stable</a><br />\n<br />\n";
			echo "Latest d2pack109 release: <input type=\"text\" name=\"d2pack109\" value=\"".$latest_d2pack109."\"><br />\n";
			echo "<a href=\"downloads.php?type=d2pack109\">Edit files for d2pack109</a><br />\n<br />\n";
			echo "<input type=\"submit\" value=\"Apply changes\">\n";
			echo "</form>\n";
		} else if ($_GET['action'] == 'dodownloads') {
			if (mysql_query("UPDATE config SET `value` = '".$_POST['development']."' WHERE `key` = 'latest_development';",$dbh) && mysql_query("UPDATE config SET `value` = '".$_POST['stable']."' WHERE `key` = 'latest_stable';",$dbh) && mysql_query("UPDATE config SET `value` = '".$_POST['d2pack109']."' WHERE `key` = 'latest_d2pack109';",$dbh)) {
				echo "<p align=\"center\">Updated successfully</p>\n";
			} else {
				echo "<p align=\"center\">Error: Could not apply changes</p>\n";
				echo "<p align=\"center\">MySQL said: ".mysql_error()."</p>\n";
			}
		} else if ($_GET['action'] == 'config') {
			$row = mysql_fetch_row($temp = mysql_query("SELECT COUNT(*) FROM news",$dbh));
			$total_newsitems = $row[0];
			mysql_free_result($temp);
			$row = mysql_fetch_row($temp = mysql_query("SELECT value+0 FROM config WHERE `key` = 'items_front_page'",$dbh));
			$items_front_page = $row[0];
			mysql_free_result($temp);
			$row = mysql_fetch_row($temp = mysql_query("SELECT value+0 FROM config WHERE `key` = 'items_archive_page'",$dbh));
			$items_archive_page = $row[0];
			mysql_free_result($temp);
			$row = mysql_fetch_row($temp = mysql_query("SELECT value FROM config WHERE `key` = 'footer'",$dbh));
			$footer = $row[0];
			mysql_free_result($temp);
			unset($temp);
			unset($row);
			echo "<p><form action=\"".$_SERVER['PHP_SELF']."?action=doconfig\" method=\"post\">\n";
			echo "<b>News items on front page:</b> <input type=\"text\" size=\"2\" name=\"items_front_page\" value=\"".$items_front_page."\"><br />\n";
			echo "<b>News items per archive page:</b> <input type=\"text\" size=\"2\" name=\"items_archive_page\" value=\"".$items_archive_page."\"><br /><br />\n";
			echo "<b>Page footer:</b><br />\n";
			echo "<textarea name=\"footer\" cols=\"50\" rows=\"5\">".$footer."</textarea><br />\n";
			echo "<input type=\"submit\" value=\"Apply changes\">\n";
			echo "</form></p>\n";
		} else if ($_GET['action'] == 'doconfig') {
			if (mysql_query("UPDATE config SET `value` = '".$_POST['items_front_page']."' WHERE `key` = 'items_front_page';",$dbh) && mysql_query("UPDATE config SET `value` = '".$_POST['items_archive_page']."' WHERE `key` = 'items_archive_page';",$dbh) && mysql_query("UPDATE config SET `value` = '".$_POST['footer']."' WHERE `key` = 'footer';",$dbh)) {
				echo "<p align=\"center\">Updated successfully</p>\n";
			} else {
				echo "<p align=\"center\">Error: Could not apply changes</p>\n";
				echo "<p align=\"center\">MySQL said: ".mysql_error()."</p>\n";
			}
		} else if ($_GET['action'] == 'logout') {
			echo "<p align=\"center\">Logout successful</p>\n";
		}
	}
}
?>
</body>
</html>
