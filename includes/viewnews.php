<?php
$row = mysql_fetch_row($temp = mysql_query("SELECT COUNT(*) FROM news",$dbh));
$total_newsitems = $row[0];
mysql_free_result($temp);
$row = mysql_fetch_row($temp = mysql_query("SELECT value+0 FROM config WHERE `key` = 'items_front_page'",$dbh));
$items_front_page = $row[0];
mysql_free_result($temp);
$row = mysql_fetch_row($temp = mysql_query("SELECT value+0 FROM config WHERE `key` = 'items_archive_page'",$dbh));
$items_archive_page = $row[0];
mysql_free_result($temp);
unset($temp);
unset($row);

$pages = round(($total_newsitems - $items_front_page) / $items_archive_page);

if ($pages == 0) {
	$pages = 1;
}

if ($_GET['page'] != 'newsarchive') {
	$limit = '0,'.$items_front_page;
} else if (!isset($_GET['start']) || $_GET['start'] > $total_newsitems) {
	$limit = $items_front_page .','. $items_archive_page;
	$curpage = 1;
} else {
	$limit = $_GET['start'] .','. $items_archive_page;
	if (!is_int($curpage = ((($_GET['start'] - $items_front_page) / $items_archive_page) + 1))) {
		$curpage = 0;
	}
}

$query = mysql_query("SELECT t1.username,t1.email,t2.timestamp,t2.subject,t2.text FROM users AS t1, news AS t2 WHERE t1.uid = t2.poster ORDER BY t2.timestamp DESC LIMIT ".$limit.";",$dbh);

if ($row = mysql_fetch_row($query)) {
	do {
		echo "				  <table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">";
		echo "				    <tr>";
		echo "				      <td align=\"left\" class=\"newstitle\">";
		echo "				        <strong>" . $row[3]. "</strong>";
		echo "				        <br /><span class=\"text9\">Posted by <a href=\"mailto:". $row[1] ."\">". $row[0] ."</a> on ".date($dateformat,$row[2])."</span><br /><br />";
		echo "				        <span class=\"text13\">" . str_replace("\n","<br />\n",str_replace("changelog","<a href=\"index.php?page=changelog\">changelog</a>",$row[4])) . "</span>";
		echo "				      </td></tr></table>";
		echo "				  <div>&nbsp;</div>";
		echo "				  <hr /><div>&nbsp;</div>";
	} while ($row = mysql_fetch_row($query));
	mysql_free_result($query);
} else {
	echo "				  <div style=\"text-align:center\">No news found</div>\n";
}
if ($_GET['page'] == 'newsarchive') {
?>
					  <table width="100%">
						<tr>
						  <td class="newsarchive">
						  	Page: 
<?php
	for ($x=0;$x<$pages;$x++) {
		if ($x+1 == $curpage) {
			echo " ".round($x+1);
		} else {
			echo " <a href=\"index.php?page=newsarchive&start=".round($x*$items_archive_page + $items_front_page)."\" class=\"link2\">".round($x+1)."</a>";
		}
	}
?>
						  </td>
						</tr>
					  </table><br />
<?php
} else {
?>
					  <table width="100%">
						<tr>
						  <td style="text-align:center">
							<a href="index.php?page=newsarchive" class="link2">News archive</a>
						  </td>
						</tr>
					  </table><br />
<?php
}	
?>
