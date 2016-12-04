<?xml kwquestionMark?>
<?php
	/***
	 * We are installing WordPress. Multiline Comment
	 ***/	
	if ( false ) 
	{
?>
		<!DOCTYPE html>
		<html xmlns="http://www.w3.org/1999/xhtml">
			<head id="style" name="xx" type=unquoted unknown="xxx">
    			<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
				<script>
					x = 12345.54;
					Y="string" 'string'
					^$\regexp
				</script>
			</head>
		</html>
	}
<?
	define( 'WP_INSTALLING', true );
	function display_header() { header( 'Content-Type: text/html; charset=utf-8' ); }
	$step = isset( $_GET['step'] ) ? (int) $_GET['step'] : 0;
	$x = array('a' => 'c');    
?>
	<style>	
		body { a = 1; 'string'; background: #FFFFFF;   } 
	</style>
