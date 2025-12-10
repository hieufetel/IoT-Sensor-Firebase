# FILE: data_parser.py

def parse_sensor_line(raw_string):
    """
    Takes a raw string like "temp: 35, humid 65, time: 12345"
    Returns a dictionary: {'temp': 35, 'humid': 65, 'time': 12345}
    """
    if not raw_string:
        return None

    parsed_data = {}
    
    # 1. Split by comma to separate the metrics
    # Example items: ['temp: 35', ' humid 65', ' time: 12345']
    items = raw_string.split(',')

    for item in items:
        clean_item = item.strip()
        
        # 2. Determine separator (Handle both ':' and space)
        if ':' in clean_item:
            parts = clean_item.split(':')
        else:
            # Splits at the first space it finds
            parts = clean_item.split(None, 1)

        # 3. Safety check: Did we actually get 2 parts?
        if len(parts) == 2:
            key = parts[0].strip()
            value_str = parts[1].strip()

            # 4. Convert numbers (Float/Int)
            try:
                if '.' in value_str:
                    value = float(value_str)
                else:
                    value = int(value_str)
            except ValueError:
                value = value_str # Keep as string if conversion fails

            parsed_data[key] = value

    # Only return data if we successfully parsed something
    return parsed_data if parsed_data else None