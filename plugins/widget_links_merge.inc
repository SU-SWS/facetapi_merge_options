<?php

/**
 * @file
 * The facetapi_links and facetapi_checkbox_links widget plugin classes.
 */

/**
 * Widget that renders facets as a list of clickable links.
 *
 * Links make it easy for users to narrow down their search results by clicking
 * on them. The render arrays use theme_item_list() to generate the HTML markup.
 */
class FacetapiWidgetLinksMerge extends FacetapiWidgetLinks {

   /**
   * Implements FacetapiWidget::execute().
   *
   * Transforms the render array into something that can be themed by
   * theme_item_list().
   *
   * @see FacetapiWidgetLinks::setThemeHooks()
   * @see FacetapiWidgetLinks::buildListItems()
   */
  public function execute() {
    $element = &$this->build[$this->facet['field alias']];
    $key = $this->getKey();

    // If we have no settings then just carry on as before.
    if (!isset($this->settings->settings['merge']['grouping'])
      || empty($this->settings->settings['merge']['grouping'])) {
      parent::execute();
      return;
    }

    $raw_settings = $this->settings->settings['merge']['grouping'];
    $merge_settings = $this->prepareMergeSettings($raw_settings);

    foreach ($merge_settings as $element_name => $values) {

      // Set some defaults from the first field to merge in the configuration.
      // $first_field = $values['fields'][0];
      // $element[$element_name]["#path"] = @$element[$first_field]['#path'];
      // $element[$element_name]["#query"] = @$element[$first_field]['#query'];

      if (count($values['fields']) <= 0) {
        parent::execute();
        return;
      }

      $i = 0;
      $first_field = NULL;
      while(isset($values['fields'][$i])){
        if (isset($element[$values['fields'][$i]])) {
          $first_field = $values['fields'][$i];
          continue;
        }
        $i++;
      }

      if (!$first_field) {
        parent::execute();
        return;
      }

      // Create a new element for our merged field.
      $element[$element_name] = $this->getElementPlaceholder();

      // Place the label directly.
      $element[$element_name]['#markup'] = $values['markup'];
      $element[$element_name]["#path"] = @$element[$first_field]['#path'];
      $element[$element_name]["#query"] = @$element[$first_field]['#query'];

      // Loop through the values and modify the query $f param.
      foreach ($values['fields'] as $element_key_name) {

        // Add up a running counter of the combined results.
        $element[$element_name]["#count"] += @$element[$element_key_name]['#count'];

        // Easy way to find active state.
        $element[$element_name]["#active"] = @$element[$element_key_name]['#active'];

        // Look and find the position in the array of the facet key.
        $needle = $key . ":" . $element_key_name;
        $haystack = @$element[$element_name]['#query']['f'];

        // No haystack. Just move along.
        if (!is_array($haystack)) {
          unset($element[$element_key_name]);
          continue;
        }

        $pos = array_search($needle, $haystack);

        // If the element is active we want to remove the f entry so that the
        // filter is removed when the user clicks on the filter.
        if (!empty($element[$element_key_name]['#active'])) {
          // If the search found the entry we need to remove it.
          if ($pos !== FALSE) {
            unset($element[$element_name]['#query']['f'][$pos]);
          }
        }
        // Else if the element is inactive and the position was not found we
        // then need to add it to the F array.
        elseif ($pos === FALSE) {
          $element[$element_name]['#query']['f'][] = $needle;
        }

        // Finally, remove the original element from being rendered.
        unset($element[$element_key_name]);
      }

      // If no results go home!
      if ($element[$element_name]["#count"] <= 0) {
        unset($element[$element_name]);
      }

    }

    // Need to re-sort the elements now that we have modified them.
    $this->applySorts($element);

    // Sets each item's theme hook, builds item list.
    $this->setThemeHooks($element);
    $element = array(
      '#theme' => 'item_list',
      '#items' => $this->buildListItems($element),
      '#attributes' => $this->build['#attributes'],
    );
  }

  /**
   * Overrides FacetapiWidgetLinks::settingsForm().
   */
  function settingsForm(&$form, &$form_state) {

    parent::settingsForm($form, $form_state);

    if (!isset($form['#submit']) || is_array($form['#submit'])) {
      $form['#submit'] = array();
    }

    array_unshift($form["#submit"], "facetapi_merge_options_settings_form_submit");

    $form['merge'] = array(
      '#type' => "fieldset",
      '#title' => t("Merge options settings"),
      '#id' => "edit-merge-wrapper",
      '#collapsed' => FALSE,
      '#collapsible' => TRUE,
      '#description' => t("Merge two or more options into one visual link"),
      '#weight' => 10,
      '#tree' => TRUE,
      '#states' => array(
        'visible' => array(
          ':input[name="widget"]' => array(
            array('value' => 'facetapi_links_merge'),
            array('value' => 'facetapi_checkbox_links_merge'),
          ),
        ),
      ),
    );

    $my_settings = isset($this->settings->settings['merge']['grouping']) ? $this->settings->settings['merge']['grouping'] : array();

    // Number of groupings
    if (empty($form_state['num_groups'])) {
      $form_state['num_groups'] = count($my_settings);
    }

    for ($delta = 0; $delta <= $form_state['num_groups']; $delta++) {

      if (isset($form_state['merge_values'][$delta])) {
        $beta = $form_state['merge_values'][$delta];
      }
      else {
        // Minus 2 because of the label field and the add another field.
        $beta = isset($my_settings[$delta]) ? count($my_settings[$delta]) - 2 : 2;
      }

      // Number of group options.
      $form['merge']['grouping'][$delta] = array(
        '#type' => 'fieldset',
        '#title' => t('Merge grouping #') . ($delta + 1),
        '#collapsed' => FALSE,
        '#collapsible' => TRUE,
        '#id' => "edit-grouping-" . $delta,
      );

      // Mandatory labels.
      $form['merge']['grouping'][$delta]['link_label'] = array(
        '#type' => 'textfield',
        '#title' => t('Label for the link'),
        '#description' => t("Enter the new name for this link"),
        '#default_value' => isset($my_settings[$delta]['link_label']) ? $my_settings[$delta]['link_label'] : "",
      );

      for ($count = 1; $count <= $beta; $count++) {
        $form['merge']['grouping'][$delta]['merge_value_' . $count] = array(
          '#type' => 'textfield',
          '#title' => t('Merge option ') . $count,
          '#description' => t("Select the value to be merged into"),
          '#default_value' => isset($my_settings[$delta]['merge_value_' . $count]) ? $my_settings[$delta]['merge_value_' . $count] : "",
        );
      }

      $form['merge']['grouping'][$delta]['add_nuther_value'] = array(
        '#type' => 'submit',
        '#value' => t('Add another option'),
        '#name' => 'add-nuther-value' . $delta,
        '#submit' => array('facetapi_merge_options_add_nuther_value'),
        '#ajax' => array(
          'callback' => 'facetapi_merge_options_add_nuther_value_callback',
          'wrapper' => 'edit-grouping-' . $delta,
        ),
      );

    }

    $form['merge']['add_nuther'] = array(
      '#type' => 'submit',
      '#value' => t('Add another grouping'),
      '#name' => 'add-nuther' . $delta,
      '#submit' => array('facetapi_merge_options_add_nuther'),
      '#ajax' => array(
        'callback' => 'facetapi_merge_options_add_nuther_callback',
        'wrapper' => 'edit-merge-wrapper',
      ),
    );


  }


  /**
   * Return an array with default values.
   * @return [type] [description]
   */
  private function getElementPlaceholder() {
    return array(
      "#markup" => "",
      "#path" => "search",
      "#html" => FALSE,
      "#indexed_value" => "",
      "#count" => 0,
      "#active" => 0,
      "#item_parents" => array(),
      "#item_children" => array(),
      "#query" => array(),
    );
  }

  /**
   * Formats the raw settings into a usable format.
   * @return [type] [description]
   */
  private function prepareMergeSettings($raw) {
    $settings = array();

    foreach ($raw as $index => $values) {
      $name = "merge_" . md5(serialize($values));
      $settings[$name]['markup'] = $values['link_label'];
      $settings[$name]['fields'] = array();
      unset($values['link_label']);
      unset($values['add_nuther_value']);
      foreach ($values as $k => $field_value) {
        if (empty($field_value)) {
          continue;
        }
        $settings[$name]['fields'][] = $field_value;
      }
    }

    return $settings;
  }

}

/**
 * Widget that renders facets as a list of clickable checkboxes.
 *
 * This widget renders facets in the same way as FacetapiWidgetLinks but uses
 * JavaScript to transform the links into checkboxes followed by the facet.
 */
class FacetapiWidgetCheckboxLinksMerge extends FacetapiWidgetLinksMerge {
  /**
   * Overrides FacetapiWidgetLinks::init().
   *
   * Adds additional JavaScript settings and CSS.
   */
  public function init() {
    parent::init();
    $this->jsSettings['makeCheckboxes'] = 1;
    drupal_add_css(drupal_get_path('module', 'facetapi') . '/facetapi.css');
  }

  /**
   * Overrides FacetapiWidgetLinks::getItemClasses().
   *
   * Sets the base class for checkbox facet items.
   */
  public function getItemClasses() {
    return array('facetapi-checkbox');
  }
}


