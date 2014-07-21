<?php

/**
 * Callback for retrieving course objects.
 *
 * @param int $id
 * @return object
 */
function _bookpages_fetch_pages($fn, $nid) {
	// neatness
  	return _build_book_response($nid);
}

// @param course_nid will be the node id of the node:couse we want
// pages from
function _build_book_response($course_nid) {
	
	// we are expecting the nid of node bundle 'course'
	// we need to know it's bid (book id)
	$bid = _get_bid($course_nid);
	
	// we also need this book's (same node) mlid (menu link id)
	// we are using D7's Book module api - it uses D7's Menu api
	// to manage page order. Page order is managed completely
	// outside of the scope of this module.
	$mlid = _get_mlid($course_nid);
	
	// this is our contact point with the book module api
	// get us a list of mlid's for each child page for this book
	$toc = _get_book_toc ($bid);
	
	// strip this array down to just the mild keys, one for each child
	$toc_keys = array_keys($toc);
	
	// drop & capture the first member - this is the node:course 
	// we started with above
	$course_parent_mild = array_shift($toc_keys);
	// now we're going to need the course_node object
	$course_node = node_load($course_nid);

	// we want the title of the couse
	$basic_course_node = _course_basic_view($course_node);


	// lets build up an array of page result to return from our api call
	$pages = array();
	
	// wratchet thru each child page
	$page_number = 0;
	foreach ($toc_keys as $key) {

		$page_number++;
		
		// we now need a nid for each child page returned 
		// from the menu api above
		$nid = _get_page_nid($key);
		
		// in this case we can get away with harvesting just one
		// attached field from the course_page node
		// the call to 
		// this is a horrible hard-wired-hack (config & module code touching)
		// but will do for now
		// (see security notes in _get_node_attached_field)
		$field_content = _get_node_attached_field($nid, 'field_content'); 
		// i just threw up!

		// now we're going to need the node object
		$node = node_load($nid);

		// we need to build up render arrays for each base field found in
		// the node entity. If we need another field - such as last update etc,
		// add each in to _course_page_basic_view().
		// @todo make sure security is handled in this chain of calls, I think it is
		$basic_node = _course_page_basic_view($node);

		// add our treasures to the page array
		$pages[] = (object)array(

			// all we built was the title field
			// so we wil get a render array for the title field
			'title' => render($basic_node['title']),

			// yeah yeah...
			'field_content' => $field_content,

			//page number would be nice
			'page_number' => $page_number,
			
			);
	}

	// @todo why do I do this (object) thing again?
	return (object)array(
		array(
			'course' => array(
				'title' => render($basic_course_node['title']),
				'quiz_ref' => intval(render($basic_course_node['field_course_quiz_ref'])),
				'page_count' => count($pages),
			),
		'pages' => (object)$pages,
		),
	);
}

function _get_node_attached_field($nid, $field_name) {
	$node = node_load($nid);
	// security handled by D7's field api, as is all formatting
	// and node access features
	// ref: http://www.computerminds.co.uk/articles/rendering-drupal-7-fields-right-way
	$content = field_view_field('node', $node, $field_name, array('label' => 'hidden'));

	return render($content);
}

function _get_page_nid($mlid) {
	
	// cross-ref a book member's actual nid by it's mlid
	$nid = db_select('book' , 'bk')
		->condition('bk.mlid' , $mlid)
		->fields('bk' , array('nid'))
		->execute()
		->fetchField();
		
	return $nid;
}

function _get_bid($nid) {
	
	// cross-ref a book member's actual bid by it's nid
	$bid = db_select('book' , 'bk')
		->condition('bk.nid' , $nid)
		->fields('bk' , array('bid'))
		->execute()
		->fetchField();
		
	return $bid;
		
}

function _get_mlid($nid) {

	// cross ref a book member's mild by it's nid
	$mlid = db_select('book' , 'bk')
		->condition('bk.nid' , $nid)
		->fields('bk' , array('mlid'))
		->execute()
		->fetchField();

	return $mlid;

}

function _get_book_toc ($bid) {
	// just a wrapper for the book module api call
	return book_toc($bid, 99); // the 99 business if for the param 'depth'
}

// ref: http://api.drupalize.me/api/drupal/function/entity_example_basic_view/7
function _course_page_basic_view($entity, $view_mode = 'full') {

	$entity->content = array(
		'#view_mode' => $view_mode,
		'#language' => 'und',
		'#label_display' => 'hidden',
	);

	// Build fields content - this where the FieldAPI really comes in to play.
	// The task has very little code here because it all gets taken care of by
	// field module.
	field_attach_prepare_view('node', array($entity->nid => $entity), $view_mode);

	entity_prepare_view('node', array($entity->nid => $entity));

	$entity->content += field_attach_view('node', $entity, 'full', array());

	// example add date code
	// 
	// $entity->content['created'] = array(
	// 	'#type' => 'item', 
	// 	'#title' => t('Created date'), 
	// 	'#markup' => format_date($entity->created),
	// );

	$entity->content['title'] = array(
		'#type' => 'item', 
		'#markup' => $entity->title,
	);

	$type = 'course_page';

	drupal_alter('node_content', $entity->content, $type);

	return $entity->content;
}

function _course_basic_view($entity, $view_mode = 'full') {

	$entity->content = array(
		'#view_mode' => $view_mode,
		'#language' => 'und',
		'#label_display' => 'hidden',
	);

	// Build fields content - this where the FieldAPI really comes in to play.
	// The task has very little code here because it all gets taken care of by
	// field module.
	field_attach_prepare_view('node', array($entity->nid => $entity), $view_mode);

	entity_prepare_view('node', array($entity->nid => $entity));

	$entity->content += field_attach_view('node', $entity, 'full', array());

	$entity->content['title'] = array(
		'#type' => 'item', 
		'#markup' => $entity->title,
	);
 
 	if (array_key_exists ( $entity->language , $entity->field_course_quiz_ref )) {
		$entity->content['field_course_quiz_ref'] = array( 
			'#markup' => $entity->field_course_quiz_ref['und'][0]['target_id'],
		);
 	}

	

	$type = 'course';

	drupal_alter('node_content', $entity->content, $type);

	return $entity->content;
}