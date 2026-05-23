module.exports = [
  { "type": "heading", "defaultValue": "Weather Settings" },
  {
    "type": "section",
    "items": [
      { "type": "input", "messageKey": "API_KEY", "label": "API Key" },
      {
        "type": "radiogroup",
        "messageKey": "UNITS",
        "label": "Units",
        "options": [
          { "label": "Imperial (F)", "value": "imperial" },
          { "label": "Metric (C)", "value": "metric" }
        ]
      },
      {
        "type": "radiogroup",
        "messageKey": "TIME_FORMAT", // <-- Make sure this is here
        "label": "Time Format",
        "options": [
          { "label": "12 Hour", "value": "12h" },
          { "label": "24 Hour", "value": "24h" }
        ]
      }
    ]
  },
  { "type": "submit", "defaultValue": "Save" }
];