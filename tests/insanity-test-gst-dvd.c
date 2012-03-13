/* Insanity QA system

 Copyright (c) 2012, Collabora Ltd
 Author: Vincent Penquerc'h <vincent@collabora.co.uk>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this program; if not, write to the
 Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <insanity-gst/insanity-gst.h>

static GstElement *global_pipeline = NULL;

static GstPipeline*
dvd_test_create_pipeline (InsanityGstPipelineTest *ptest, gpointer userdata)
{
  GstElement *pipeline = NULL, *playbin2 = NULL;
  const char *launch_line = "playbin2 name=foo audio-sink=fakesink video-sink=fakesink";
  GError *error = NULL;

  pipeline = gst_parse_launch (launch_line, &error);
  if (!pipeline) {
    insanity_test_validate_step (INSANITY_TEST (ptest), "valid-pipeline", FALSE,
      error ? error->message : NULL);
    if (error)
      g_error_free (error);
    return NULL;
  }
  else if (error) {
    /* Do we get a dangling pointer here ? gst-launch.c does not unref */
    pipeline = NULL;
    insanity_test_validate_step (INSANITY_TEST (ptest), "valid-pipeline", FALSE,
      error->message);
    g_error_free (error);
    return NULL;
  }

  global_pipeline = pipeline;

  return GST_PIPELINE (pipeline);
}

static gboolean
dvd_test_start(InsanityTest *test)
{
  InsanityGstPipelineTest *ptest = INSANITY_GST_PIPELINE_TEST (test);
  GValue uri = {0};
  const char *protocol;

  if (!insanity_test_get_argument (test, "uri", &uri))
    return FALSE;
  if (!strcmp (g_value_get_string (&uri), "")) {
    insanity_test_validate_step (test, "valid-pipeline", FALSE, "No URI to test on");
    g_value_unset (&uri);
    return FALSE;
  }

  if (!gst_uri_is_valid (g_value_get_string (&uri))) {
    insanity_test_validate_step (test, "uri-is-dvd", FALSE, NULL);
    g_value_unset (&uri);
    return FALSE;
  }
  protocol = gst_uri_get_protocol (g_value_get_string (&uri));
  if (!protocol || g_ascii_strcasecmp (protocol, "dvd")) {
    insanity_test_validate_step (test, "uri-is-dvd", FALSE, NULL);
    g_value_unset (&uri);
    return FALSE;
  }
  insanity_test_validate_step (test, "uri-is-dvd", TRUE, NULL);

  g_object_set (global_pipeline, "uri", g_value_get_string (&uri), NULL);
  g_value_unset (&uri);

  if (gst_element_set_state (global_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    return FALSE;
  }
  if (gst_element_get_state (global_pipeline, NULL, NULL, GST_SECOND) == GST_STATE_CHANGE_FAILURE) {
    return FALSE;
  }

  return TRUE;
}

int
main (int argc, char **argv)
{
  InsanityGstPipelineTest *ptest;
  InsanityTest *test;
  gboolean ret;
  GValue vdef = {0};

  g_type_init ();

  ptest = insanity_gst_pipeline_test_new ("dvd-test", "Tests DVD specific features", NULL);
  test = INSANITY_TEST (ptest);

  g_value_init (&vdef, G_TYPE_STRING);
  g_value_set_string (&vdef, "");
  insanity_test_add_argument (test, "uri", "The ISO to test on (dvd:///path/to/iso)", NULL, FALSE, &vdef);
  g_value_unset (&vdef);

  insanity_test_add_checklist_item (test, "uri-is-dvd", "The URI is a DVD specific URI", NULL);

  insanity_gst_pipeline_test_set_create_pipeline_function (ptest,
      &dvd_test_create_pipeline, NULL, NULL);
  insanity_gst_pipeline_test_set_initial_state (ptest, GST_STATE_READY);
  g_signal_connect_after (test, "start", G_CALLBACK (&dvd_test_start), 0);

  ret = insanity_test_run (test, &argc, &argv);

  g_object_unref (test);

  return ret ? 0 : 1;
}