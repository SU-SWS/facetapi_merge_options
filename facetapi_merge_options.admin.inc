<?php
/**
 *
 */

/**
 * [facetapi_merge_options_add_nuther_callback description]
 * @return [type] [description]
 */
function facetapi_merge_options_add_nuther_callback($form, $form_state) {
  return $form['merge'];
}

/**
 * Submit handler for the "add-one-more" button.
 *
 * Increments the max counter and causes a rebuild.
 */
function facetapi_merge_options_add_nuther($form, &$form_state) {
  $form_state['num_groups']++;
  $form_state['rebuild'] = TRUE;
}


/**
 * Validate and remove bad merge groupings
 * @param  [type] $form        [description]
 * @param  [type] &$form_state [description]
 * @return [type]              [description]
 */
function facetapi_merge_options_settings_form_submit($form, &$form_state) {
  $values = $form_state['values'];

  foreach ($values['merge']['grouping'] as $delta => $settings) {
    if (empty($settings['link_label'])) {
      unset($form_state['values']['merge']['grouping'][$delta]);
    }
  }

  // Reset the keys.
  $new_values = array_values($form_state['values']['merge']['grouping']);
  form_set_value($form['merge']['grouping'], $new_values, $form_state);
}
