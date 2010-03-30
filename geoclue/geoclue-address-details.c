/*
 * Geoclue
 * geoclue-address-details.c - Helper functions for GeoclueAddress
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <stdio.h>
#include <glib.h>

char *countries[][2] = {
	{"AF", "Afghanistan"},
	{"AX", "Aland Islands"},
	{"AL", "Albania"},
	{"DZ", "Algeria"},
	{"AS", "American Samoa"},
	{"AD", "Andorra"},
	{"AO", "Angola"},
	{"AI", "Anguilla"},
	{"AQ", "Antarctica"},
	{"AG", "Antigua and Barbuda"},
	{"AR", "Argentina"},
	{"AM", "Armenia"},
	{"AW", "Aruba"},
	{"AU", "Australia"},
	{"AT", "Austria"},
	{"AZ", "Azerbaijan"},
	{"BS", "Bahamas"},
	{"BH", "Bahrain"},
	{"BD", "Bangladesh"},
	{"BB", "Barbados"},
	{"BY", "Belarus"},
	{"BE", "Belgium"},
	{"BZ", "Belize"},
	{"BJ", "Benin"},
	{"BM", "Bermuda"},
	{"BT", "Bhutan"},
	{"BO", "Bolivia"},
	{"BA", "Bosnia and Herzegovina"},
	{"BW", "Botswana"},
	{"BV", "Bouvet Island"},
	{"BR", "Brazil"},
	{"IO", "British Indian Ocean Territory"},
	{"BN", "Brunei Darussalam"},
	{"BG", "Bulgaria"},
	{"BF", "Burkina Faso"},
	{"BI", "Burundi"},
	{"KH", "Cambodia"},
	{"CM", "Cameroon"},
	{"CA", "Canada"},
	{"CV", "Cape Verde"},
	{"KY", "Cayman Islands"},
	{"CF", "Central African Republic"},
	{"TD", "Chad"},
	{"CL", "Chile"},
	{"CN", "China"},
	{"CX", "Christmas Island"},
	{"CC", "Cocos (Keeling) Islands"},
	{"CO", "Colombia"},
	{"KM", "Comoros"},
	{"CG", "Congo"},
	{"CD", "Democratic Republic of Congo"},
	{"CK", "Cook Islands"},
	{"CR", "Costa Rica"},
	{"CI", "Cote d'Ivoire"},
	{"HR", "Croatia"},
	{"CU", "Cuba"},
	{"CY", "Cyprus"},
	{"CZ", "Czech"},
	{"DK", "Denmark"},
	{"DJ", "Djibouti"},
	{"DM", "Dominica"},
	{"DO", "Dominican"},
	{"EC", "Ecuador"},
	{"EG", "Egypt"},
	{"SV", "El Salvador"},
	{"GQ", "Equatorial Guinea"},
	{"ER", "Eritrea"},
	{"EE", "Estonia"},
	{"ET", "Ethiopia"},
	{"FK", "Falkland Islands"},
	{"FO", "Faroe Islands"},
	{"FJ", "Fiji"},
	{"FI", "Finland"},
	{"FR", "France"},
	{"GF", "French Guiana"},
	{"PF", "French Polynesia"},
	{"TF", "French Southern Territories"},
	{"GA", "Gabon"},
	{"GM", "Gambia"},
	{"GE", "Georgia"},
	{"DE", "Germany"},
	{"GH", "Ghana"},
	{"GI", "Gibraltar"},
	{"GR", "Greece"},
	{"GL", "Greenland"},
	{"GD", "Grenada"},
	{"GP", "Guadeloupe"},
	{"GU", "Guam"},
	{"GT", "Guatemala"},
	{"GG", "Guernsey"},
	{"GN", "Guinea"},
	{"GW", "Guinea-Bissau"},
	{"GY", "Guyana"},
	{"HT", "Haiti"},
	{"HM", "Heard Island and McDonald Islands"},
	{"VA", "Vatican"},
	{"HN", "Honduras"},
	{"HK", "Hong Kong"},
	{"HU", "Hungary"},
	{"IS", "Iceland"},
	{"IN", "India"},
	{"ID", "Indonesia"},
	{"IR", "Iran"},
	{"IQ", "Iraq"},
	{"IE", "Ireland"},
	{"IM", "Isle of Man"},
	{"IL", "Israel"},
	{"IT", "Italy"},
	{"JM", "Jamaica"},
	{"JP", "Japan"},
	{"JE", "Jersey"},
	{"JO", "Jordan"},
	{"KZ", "Kazakhstan"},
	{"KE", "Kenya"},
	{"KI", "Kiribati"},
	{"KP", "Democratic People's Republic of Korea"},
	{"KR", "Korea"},
	{"KW", "Kuwait"},
	{"KG", "Kyrgyzstan"},
	{"LA", "Lao"},
	{"LV", "Latvia"},
	{"LB", "Lebanon"},
	{"LS", "Lesotho"},
	{"LR", "Liberia"},
	{"LY", "Libya"},
	{"LI", "Liechtenstein"},
	{"LT", "Lithuania"},
	{"LU", "Luxembourg"},
	{"MO", "Macao"},
	{"MK", "Macedonia"},
	{"MG", "Madagascar"},
	{"MW", "Malawi"},
	{"MY", "Malaysia"},
	{"MV", "Maldives"},
	{"ML", "Mali"},
	{"MT", "Malta"},
	{"MH", "Marshall Islands"},
	{"MQ", "Martinique"},
	{"MR", "Mauritania"},
	{"MU", "Mauritius"},
	{"YT", "Mayotte"},
	{"MX", "Mexico"},
	{"FM", "Micronesia"},
	{"MD", "Moldova"},
	{"MC", "Monaco"},
	{"MN", "Mongolia"},
	{"ME", "Montenegro"},
	{"MS", "Montserrat"},
	{"MA", "Morocco"},
	{"MZ", "Mozambique"},
	{"MM", "Myanmar"},
	{"NA", "Namibia"},
	{"NR", "Nauru"},
	{"NP", "Nepal"},
	{"NL", "Netherlands"},
	{"AN", "Netherlands Antilles"},
	{"NC", "New Caledonia"},
	{"NZ", "New Zealand"},
	{"NI", "Nicaragua"},
	{"NE", "Niger"},
	{"NG", "Nigeria"},
	{"NU", "Niue"},
	{"NF", "Norfolk Island"},
	{"MP", "Northern Mariana Islands"},
	{"NO", "Norway"},
	{"OM", "Oman"},
	{"PK", "Pakistan"},
	{"PW", "Palau"},
	{"PS", "Palestinian Territory"},
	{"PA", "Panama"},
	{"PG", "Papua New Guinea"},
	{"PY", "Paraguay"},
	{"PE", "Peru"},
	{"PH", "Philippines"},
	{"PN", "Pitcairn"},
	{"PL", "Poland"},
	{"PT", "Portugal"},
	{"PR", "Puerto Rico"},
	{"QA", "Qatar"},
	{"RE", "Reunion"},
	{"RO", "Romania"},
	{"RU", "Russia"},
	{"RW", "Rwanda"},
	{"BL", "Saint Barthélemy"},
	{"SH", "Saint Helena"},
	{"KN", "Saint Kitts and Nevis"},
	{"LC", "Saint Lucia"},
	{"MF", "Saint Martin"},
	{"PM", "Saint Pierre and Miquelon"},
	{"VC", "Saint Vincent and the Grenadines"},
	{"WS", "Samoa"},
	{"SM", "San Marino"},
	{"ST", "Sao Tome and Principe"},
	{"SA", "Saudi Arabia"},
	{"SN", "Senegal"},
	{"RS", "Serbia"},
	{"SC", "Seychelles"},
	{"SL", "Sierra Leone"},
	{"SG", "Singapore"},
	{"SK", "Slovakia"},
	{"SI", "Slovenia"},
	{"SB", "Solomon Islands"},
	{"SO", "Somalia"},
	{"ZA", "South Africa"},
	{"GS", "South Georgia and the South Sandwich Islands"},
	{"ES", "Spain"},
	{"LK", "Sri Lanka"},
	{"SD", "Sudan"},
	{"SR", "Suriname"},
	{"SJ", "Svalbard and Jan Mayen"},
	{"SZ", "Swaziland"},
	{"SE", "Sweden"},
	{"CH", "Switzerland"},
	{"SY", "Syria"},
	{"TW", "Taiwan"},
	{"TJ", "Tajikistan"},
	{"TZ", "Tanzania"},
	{"TH", "Thailand"},
	{"TL", "Timor-Leste"},
	{"TG", "Togo"},
	{"TK", "Tokelau"},
	{"TO", "Tonga"},
	{"TT", "Trinidad and Tobago"},
	{"TN", "Tunisia"},
	{"TR", "Turkey"},
	{"TM", "Turkmenistan"},
	{"TC", "Turks and Caicos Islands"},
	{"TV", "Tuvalu"},
	{"UG", "Uganda"},
	{"UA", "Ukraine"},
	{"AE", "United Arab Emirates"},
	{"GB", "United Kingdom"},
	{"US", "United States"},
	{"UM", "United States"},  /* US Minor Outlying Islands */
	{"UY", "Uruguay"},
	{"UZ", "Uzbekistan"},
	{"VU", "Vanuatu"},
	{"VE", "Venezuela"},
	{"VN", "Viet Nam"},
	{"VG", "Virgin Islands"}, /* British */
	{"VI", "Virgin Islands"}, /* US */
	{"WF", "Wallis and Futuna"},
	{"EH", "Western Sahara"},
	{"YE", "Yemen"},
	{"ZM", "Zambia"},
	{"ZW", "Zimbabwe"},
	{NULL, NULL},
};


/**
 * SECTION:geoclue-address-details
 * @short_description: Convenience functions for handling Geoclue address
 * #GHashTables
 */
#include <geoclue/geoclue-types.h>
#include "geoclue-address-details.h"

/**
 * geoclue_address_details_new:
 * 
 * Creates a new #GHashTable suitable for Geoclue Address details.
 * Both keys and values inserted to this #GHashTable will be freed
 * on g_hash_table_destroy().
 * 
 * Return value: New #GHashTable
 */
GHashTable *
geoclue_address_details_new ()
{
	return g_hash_table_new_full (g_str_hash, g_str_equal,
	                              g_free, g_free);
}


/**
 * geoclue_address_details_insert:
 * @address: #GHashTable to insert value in
 * @key: the key to use, one of GEOCLUE_ADDRESS_KEY_*
 * @value: value to insert into address
 * 
 * Adds a address field into @address. Will take copies
 * of the strings.
 */
void
geoclue_address_details_insert (GHashTable *address,
                                const char *key, const char *value)
{
	g_hash_table_insert (address, g_strdup (key), g_strdup (value));
}

static void
copy_address_key_and_value (char *key, char *value, GHashTable *target)
{
	geoclue_address_details_insert (target, key, value);
}


/**
 * geoclue_address_details_copy:
 * @source: #GHashTable to copy
 * 
 * Deep-copies a #GHashTable.
 * 
 * Return value: New, deep copied #GHashTable
 */
GHashTable *
geoclue_address_details_copy (GHashTable *source)
{
	GHashTable *target;
	
	g_assert (source != NULL);
	
	target = geoclue_address_details_new ();
	g_hash_table_foreach (source, 
	                      (GHFunc)copy_address_key_and_value, 
	                      target);
	return target;
}


/**
 * geoclue_address_details_set_country_from_code:
 * @address: #GHashTable with address data
 * 
 * Uses the "ISO 3166-1 alpha-2" list to figure out the country name matching 
 * the country code in @details, and adds the country name to details.
 * 
 * Using this function in providers is useful even when the data source includes
 * country name: this way names are standardized.
 */
void
geoclue_address_details_set_country_from_code (GHashTable *address)
{
	static GHashTable *country_table = NULL;
	const char *code;
	const char *country = NULL;

	if (!country_table) {
		int i;

		country_table = g_hash_table_new (g_str_hash, g_str_equal);
		for (i = 0; countries[i][0]; i++) {
			g_hash_table_insert (country_table, countries[i][0], countries[i][1]);
		}
	}

	code = g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRYCODE);
	if (code) {
		char *upper = g_ascii_strup (code, -1);
		country = g_hash_table_lookup (country_table, upper);
		g_free (upper);
	}

	if (country) {
		geoclue_address_details_insert (
		    address, GEOCLUE_ADDRESS_KEY_COUNTRY, country);
	} else {
		g_hash_table_remove (address, GEOCLUE_ADDRESS_KEY_COUNTRY);
	}
}

/**
 * geoclue_address_details_get_accuracy_level:
 * @address: A #GHashTable with address hash values
 * 
 * Returns a #GeoclueAccuracy that best describes the accuracy of @address
 * 
 * Return value: #GeoclueAccuracy
 */
GeoclueAccuracyLevel
geoclue_address_details_get_accuracy_level (GHashTable *address)
{
	if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_STREET)) {
		return GEOCLUE_ACCURACY_LEVEL_STREET;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_POSTALCODE)) {
		return GEOCLUE_ACCURACY_LEVEL_POSTALCODE;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_LOCALITY)) {
		return GEOCLUE_ACCURACY_LEVEL_LOCALITY;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_REGION)) {
		return GEOCLUE_ACCURACY_LEVEL_REGION;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRY) ||
	           g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRYCODE)) {
		return GEOCLUE_ACCURACY_LEVEL_COUNTRY;
	}
	return GEOCLUE_ACCURACY_LEVEL_NONE;
}
