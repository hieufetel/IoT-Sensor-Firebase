def parse_sensor_line(raw_string):
    if not raw_string:
        return None

    parsed_data = {}
    items = raw_string.split(',')

    for item in items:
        clean_item = item.strip()

        if ':' in clean_item:
            parts = clean_item.split(':')
        else:
            parts = clean_item.split(None, 1)

        if len(parts) == 2:
            key = parts[0].strip()
            value_str = parts[1].strip()

            try:
                value = float(value_str) if '.' in value_str else int(value_str)
            except:
                value = value_str

            parsed_data[key] = value

    return parsed_data if parsed_data else None
